/* The core of the hypervisor: create a KVM virtual machine, give it memory and
 * a vCPU, put the vCPU in real mode, and run it — servicing the VM exits the
 * guest triggers (port I/O, HLT, and the error exits worth reporting). */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/kvm.h>

#include "cpuid.h"
#include "ioport.h"
#include "mmio.h"
#include "serial.h"
#include "vmm.h"

#define PAGE_SIZE       0x1000ULL
#define PAGE_TABLE_PML4 0x2000ULL
#define PAGE_TABLE_PDPT 0x3000ULL
#define PAGE_TABLE_PD   0x4000ULL
#define GDT_ADDRESS     0x5000ULL
#define LONG_MODE_STACK 0x800000ULL

#define PAGE_PRESENT    (1ULL << 0)
#define PAGE_WRITABLE   (1ULL << 1)
#define PAGE_LARGE      (1ULL << 7)

#define CR0_PE          (1ULL << 0)
#define CR0_PG          (1ULL << 31)
#define CR4_PAE         (1ULL << 5)
#define EFER_LME        (1ULL << 8)
#define EFER_LMA        (1ULL << 10)

#define GDT_CODE_SELECTOR 0x08U
#define GDT_DATA_SELECTOR 0x10U
#define GDT_CODE_ENTRY    0x00AF9B000000FFFFULL
#define GDT_DATA_ENTRY    0x00CF93000000FFFFULL

int vm_create(struct vm *vm, size_t mem_size) {
    memset(vm, 0, sizeof(*vm));
    vm->kvm_fd = -1;
    vm->vm_fd = -1;
    vm->vcpu_fd = -1;
    vm->mem = MAP_FAILED;
    vm->run = MAP_FAILED;
    vm->mem_size = mem_size;

    vm->kvm_fd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (vm->kvm_fd < 0) {
        perror("open /dev/kvm (run ./scripts/setup-kvm.sh?)");
        return -1;
    }

    int api = ioctl(vm->kvm_fd, KVM_GET_API_VERSION, 0);
    if (api != KVM_API_VERSION) {
        fprintf(stderr, "unexpected KVM API version %d (want %d)\n",
                api, KVM_API_VERSION);
        return -1;
    }

    vm->vm_fd = ioctl(vm->kvm_fd, KVM_CREATE_VM, 0);
    if (vm->vm_fd < 0) { perror("KVM_CREATE_VM"); return -1; }

    vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (vm->mem == MAP_FAILED) { perror("mmap guest memory"); return -1; }

    struct kvm_userspace_memory_region region = {
        .slot = 0,
        .guest_phys_addr = 0,
        .memory_size = mem_size,
        .userspace_addr = (uint64_t)(uintptr_t)vm->mem,
    };
    if (ioctl(vm->vm_fd, KVM_SET_USER_MEMORY_REGION, &region) < 0) {
        perror("KVM_SET_USER_MEMORY_REGION");
        return -1;
    }

    vm->vcpu_fd = ioctl(vm->vm_fd, KVM_CREATE_VCPU, 0);
    if (vm->vcpu_fd < 0) { perror("KVM_CREATE_VCPU"); return -1; }
    if (cpuid_setup(vm) < 0)
        return -1;

    int run_size = ioctl(vm->kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (run_size < 0) { perror("KVM_GET_VCPU_MMAP_SIZE"); return -1; }
    vm->run_size = (size_t)run_size;

    vm->run = mmap(NULL, vm->run_size, PROT_READ | PROT_WRITE,
                   MAP_SHARED, vm->vcpu_fd, 0);
    if (vm->run == MAP_FAILED) { perror("mmap kvm_run"); return -1; }

    serial_reset();
    return 0;
}

int vm_load_guest(struct vm *vm, uint64_t guest_addr, const void *code,
                  size_t len) {
    if (guest_addr > vm->mem_size || len > vm->mem_size - guest_addr) {
        fprintf(stderr, "guest image does not fit in %zu bytes of RAM\n",
                vm->mem_size);
        return -1;
    }
    memcpy(vm->mem + guest_addr, code, len);
    return 0;
}

int vm_set_real_mode(struct vm *vm, uint64_t entry) {
    struct kvm_sregs sregs;

    if (ioctl(vm->vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
        perror("KVM_GET_SREGS");
        return -1;
    }
    /* After reset the code segment points at the top of memory; rebase it to 0
     * so a flat real-mode `rip` addresses guest-physical memory directly. The
     * data segments already reset with base 0. */
    sregs.cs.base = 0;
    sregs.cs.selector = 0;
    if (ioctl(vm->vcpu_fd, KVM_SET_SREGS, &sregs) < 0) {
        perror("KVM_SET_SREGS");
        return -1;
    }

    struct kvm_regs regs = {
        .rip = entry,
        .rsp = 0x0,
        .rflags = 0x2, /* bit 1 is always set; DF=0 so LODSB counts up */
    };
    if (ioctl(vm->vcpu_fd, KVM_SET_REGS, &regs) < 0) {
        perror("KVM_SET_REGS");
        return -1;
    }
    return 0;
}

static struct kvm_segment code_segment(void) {
    return (struct kvm_segment) {
        .base = 0,
        .limit = UINT32_MAX,
        .selector = GDT_CODE_SELECTOR,
        .type = 11,
        .present = 1,
        .dpl = 0,
        .db = 0,
        .s = 1,
        .l = 1,
        .g = 1,
    };
}

static struct kvm_segment data_segment(void) {
    return (struct kvm_segment) {
        .base = 0,
        .limit = UINT32_MAX,
        .selector = GDT_DATA_SELECTOR,
        .type = 3,
        .present = 1,
        .dpl = 0,
        .db = 1,
        .s = 1,
        .l = 0,
        .g = 1,
    };
}

static int build_long_mode_tables(struct vm *vm) {
    if (vm->mem_size < LONG_MODE_STACK + PAGE_SIZE) {
        fprintf(stderr, "long mode requires at least 0x%llx bytes of RAM\n",
                (unsigned long long)(LONG_MODE_STACK + PAGE_SIZE));
        return -1;
    }

    uint64_t *pml4 = (uint64_t *)(vm->mem + PAGE_TABLE_PML4);
    uint64_t *pdpt = (uint64_t *)(vm->mem + PAGE_TABLE_PDPT);
    uint64_t *pd = (uint64_t *)(vm->mem + PAGE_TABLE_PD);
    uint64_t *gdt = (uint64_t *)(vm->mem + GDT_ADDRESS);

    memset(pml4, 0, PAGE_SIZE);
    memset(pdpt, 0, PAGE_SIZE);
    memset(pd, 0, PAGE_SIZE);
    memset(gdt, 0, PAGE_SIZE);

    pml4[0] = PAGE_TABLE_PDPT | PAGE_PRESENT | PAGE_WRITABLE;
    pdpt[0] = PAGE_TABLE_PD | PAGE_PRESENT | PAGE_WRITABLE;
    for (uint64_t i = 0; i < 512; i++)
        pd[i] = (i << 21) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_LARGE;

    gdt[0] = 0;
    gdt[1] = GDT_CODE_ENTRY;
    gdt[2] = GDT_DATA_ENTRY;
    return 0;
}

int vm_set_long_mode(struct vm *vm, uint64_t entry) {
    struct kvm_sregs sregs;
    struct kvm_regs regs = {
        .rip = entry,
        .rsp = LONG_MODE_STACK,
        .rbp = LONG_MODE_STACK,
        .rflags = 0x2,
    };

    if (entry >= vm->mem_size) {
        fprintf(stderr, "long-mode entry 0x%llx is outside guest RAM\n",
                (unsigned long long)entry);
        return -1;
    }
    if (build_long_mode_tables(vm) < 0)
        return -1;
    if (ioctl(vm->vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
        perror("KVM_GET_SREGS");
        return -1;
    }

    /*
     * Reproducible long-mode entry state:
     *   cr0.PE=1, cr0.PG=1, cr4.PAE=1,
     *   efer.LME=1, efer.LMA=1, cs.L=1, cs.DB=0.
     * cr3 points at a PML4 -> PDPT -> 2 MiB-page PD identity mapping 1 GiB.
     */
    sregs.cr3 = PAGE_TABLE_PML4;
    sregs.cr4 |= CR4_PAE;
    sregs.cr0 |= CR0_PE | CR0_PG;
    sregs.efer |= EFER_LME | EFER_LMA;
    sregs.gdt.base = GDT_ADDRESS;
    sregs.gdt.limit = (3U * sizeof(uint64_t)) - 1U;
    sregs.cs = code_segment();
    sregs.ds = data_segment();
    sregs.es = data_segment();
    sregs.fs = data_segment();
    sregs.gs = data_segment();
    sregs.ss = data_segment();

    if (ioctl(vm->vcpu_fd, KVM_SET_SREGS, &sregs) < 0) {
        perror("KVM_SET_SREGS");
        return -1;
    }
    if (ioctl(vm->vcpu_fd, KVM_SET_REGS, &regs) < 0) {
        perror("KVM_SET_REGS");
        return -1;
    }
    return 0;
}

static void vm_dump_registers(struct vm *vm) {
    struct kvm_regs regs;
    struct kvm_sregs sregs;

    if (ioctl(vm->vcpu_fd, KVM_GET_REGS, &regs) < 0) {
        perror("KVM_GET_REGS while dumping state");
        return;
    }
    if (ioctl(vm->vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
        perror("KVM_GET_SREGS while dumping state");
        return;
    }

    fprintf(stderr,
            "registers: rip=0x%llx rsp=0x%llx rflags=0x%llx "
            "cs=0x%x base=0x%llx\n",
            (unsigned long long)regs.rip,
            (unsigned long long)regs.rsp,
            (unsigned long long)regs.rflags,
            sregs.cs.selector,
            (unsigned long long)sregs.cs.base);
    fprintf(stderr,
            "control: cr0=0x%llx cr3=0x%llx cr4=0x%llx efer=0x%llx\n",
            (unsigned long long)sregs.cr0,
            (unsigned long long)sregs.cr3,
            (unsigned long long)sregs.cr4,
            (unsigned long long)sregs.efer);
}

int vm_run(struct vm *vm) {
    for (;;) {
        if (ioctl(vm->vcpu_fd, KVM_RUN, 0) < 0) {
            perror("KVM_RUN");
            return -1;
        }

        switch (vm->run->exit_reason) {
        case KVM_EXIT_HLT:
            return 0;

        case KVM_EXIT_IO: {
            uint16_t port = vm->run->io.port;
            uint8_t *data = (uint8_t *)vm->run + vm->run->io.data_offset;
            uint8_t size = vm->run->io.size;

            if (size == 0 || size > sizeof(uint32_t)) {
                fprintf(stderr, "invalid I/O access size=%u\n", size);
                return -1;
            }
            for (uint32_t i = 0; i < vm->run->io.count; i++) {
                uint8_t *item = data + (size_t)i * size;
                uint32_t value = 0;

                memcpy(&value, item, size);
                if (vm->run->io.direction == KVM_EXIT_IO_OUT) {
                    if (!ioport_write(port, size, value)) {
                        fprintf(stderr, "unhandled I/O write port=0x%x size=%u\n",
                                port, size);
                        return -1;
                    }
                } else {
                    if (!ioport_read(port, size, &value)) {
                        fprintf(stderr, "unhandled I/O read port=0x%x size=%u\n",
                                port, size);
                        return -1;
                    }
                    memcpy(item, &value, size);
                }
            }
            break;
        }

        case KVM_EXIT_MMIO: {
            uint32_t exit_status = 0;
            enum mmio_result result = mmio_access(
                vm->run->mmio.phys_addr,
                vm->run->mmio.is_write != 0,
                vm->run->mmio.data,
                vm->run->mmio.len,
                &exit_status);

            if (result == MMIO_UNHANDLED) {
                fprintf(stderr,
                        "unhandled MMIO %s address=0x%llx len=%u\n",
                        vm->run->mmio.is_write ? "write" : "read",
                        (unsigned long long)vm->run->mmio.phys_addr,
                        vm->run->mmio.len);
                return -1;
            }
            if (result == MMIO_STOP) {
                fprintf(stderr, "nutvisor: guest requested exit status=%u\n",
                        exit_status);
                return exit_status == 0 ? VM_RUN_EXITED : VM_RUN_ERROR;
            }
            break;
        }

        case KVM_EXIT_SHUTDOWN:
            fprintf(stderr, "guest shutdown (triple fault?)\n");
            vm_dump_registers(vm);
            return -1;

        case KVM_EXIT_FAIL_ENTRY:
            fprintf(stderr, "KVM_EXIT_FAIL_ENTRY reason=0x%llx\n",
                    (unsigned long long)
                        vm->run->fail_entry.hardware_entry_failure_reason);
            vm_dump_registers(vm);
            return -1;

        case KVM_EXIT_INTERNAL_ERROR:
            fprintf(stderr, "KVM internal error suberror=%u data-count=%u\n",
                    vm->run->internal.suberror, vm->run->internal.ndata);
            vm_dump_registers(vm);
            return -1;

        case KVM_EXIT_DEBUG:
            fprintf(stderr, "KVM_EXIT_DEBUG at pc=0x%llx\n",
                    (unsigned long long)vm->run->debug.arch.pc);
            vm_dump_registers(vm);
            return -1;

        case KVM_EXIT_EXCEPTION:
            fprintf(stderr, "KVM_EXIT_EXCEPTION vector=%u error=0x%x\n",
                    vm->run->ex.exception, vm->run->ex.error_code);
            vm_dump_registers(vm);
            return -1;

        default:
            fprintf(stderr, "unsupported KVM exit_reason=%u\n",
                    vm->run->exit_reason);
            vm_dump_registers(vm);
            return -1;
        }
    }
}

void vm_destroy(struct vm *vm) {
    if (vm->run && vm->run != MAP_FAILED)
        munmap(vm->run, vm->run_size);
    if (vm->mem && vm->mem != MAP_FAILED)
        munmap(vm->mem, vm->mem_size);
    if (vm->vcpu_fd >= 0)
        close(vm->vcpu_fd);
    if (vm->vm_fd >= 0)
        close(vm->vm_fd);
    if (vm->kvm_fd >= 0)
        close(vm->kvm_fd);
}
