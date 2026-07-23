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

#include "serial.h"
#include "vmm.h"

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

    int run_size = ioctl(vm->kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (run_size < 0) { perror("KVM_GET_VCPU_MMAP_SIZE"); return -1; }
    vm->run_size = (size_t)run_size;

    vm->run = mmap(NULL, vm->run_size, PROT_READ | PROT_WRITE,
                   MAP_SHARED, vm->vcpu_fd, 0);
    if (vm->run == MAP_FAILED) { perror("mmap kvm_run"); return -1; }

    return 0;
}

int vm_load_guest(struct vm *vm, uint64_t guest_addr, const void *code,
                  size_t len) {
    if (guest_addr + len > vm->mem_size) {
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
            if (serial_handles_port(port)) {
                for (uint32_t i = 0; i < vm->run->io.count; i++) {
                    if (vm->run->io.direction == KVM_EXIT_IO_OUT)
                        serial_out(port, data[i * vm->run->io.size]);
                    else
                        data[i * vm->run->io.size] = serial_in(port);
                }
            }
            break;
        }

        case KVM_EXIT_MMIO:
            /* Milestone 3 wires up memory-mapped devices here. */
            fprintf(stderr, "unhandled MMIO at 0x%llx\n",
                    (unsigned long long)vm->run->mmio.phys_addr);
            break;

        case KVM_EXIT_SHUTDOWN:
            fprintf(stderr, "guest shutdown (triple fault?)\n");
            return -1;

        case KVM_EXIT_FAIL_ENTRY:
            fprintf(stderr, "KVM_EXIT_FAIL_ENTRY reason=0x%llx\n",
                    (unsigned long long)
                        vm->run->fail_entry.hardware_entry_failure_reason);
            return -1;

        case KVM_EXIT_INTERNAL_ERROR:
            fprintf(stderr, "KVM internal error suberror=%u\n",
                    vm->run->internal.suberror);
            return -1;

        default:
            fprintf(stderr, "unhandled exit_reason=%u\n",
                    vm->run->exit_reason);
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
