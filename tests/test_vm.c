/* No /dev/kvm required: gcc -std=gnu11 -Wall -Wextra -Werror -Iinclude
 * tests/test_vm.c src/vm.c -o /tmp/test_vm && /tmp/test_vm */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "cpuid.h"
#include "ioport.h"
#include "mmio.h"
#include "serial.h"
#include "vmm.h"

static unsigned calls;
static uint32_t values[16];
static bool accept_io = true;

int cpuid_setup(struct vm *vm) { (void)vm; return 0; }
void serial_reset(void) {}
enum mmio_result mmio_access(uint64_t address, bool write, uint8_t *data,
                             size_t len, uint32_t *status) {
    (void)address; (void)write; (void)data; (void)len; (void)status;
    return MMIO_UNHANDLED;
}
bool ioport_write(uint16_t port, uint8_t size, uint32_t value) {
    assert(port == 0x3f8 && (size == 1 || size == 2 || size == 4));
    assert(calls < 16);
    values[calls++] = value;
    return accept_io;
}
bool ioport_read(uint16_t port, uint8_t size, uint32_t *value) {
    assert(port == 0x3f8 && (size == 1 || size == 2 || size == 4));
    *value = 0x12345678U + calls++;
    return accept_io;
}

static void prepare(struct vm *vm) {
    memset(vm->run, 0, vm->run_size);
    vm->run->exit_reason = KVM_EXIT_IO;
    vm->run->io.port = 0x3f8;
    vm->run->io.direction = KVM_EXIT_IO_OUT;
    vm->run->io.size = 1;
    vm->run->io.count = 1;
    vm->run->io.data_offset = sizeof(struct kvm_run);
    calls = 0;
}

static void reject(struct vm *vm) {
    assert(vm_handle_io(vm) == -1);
    assert(calls == 0);
}

static void test_io(void) {
    struct vm vm = { .run_size = sizeof(struct kvm_run) + 33 };
    vm.run = malloc(vm.run_size);
    assert(vm.run);
    for (unsigned size = 1; size <= 4; size *= 2) {
        prepare(&vm);
        vm.run->io.size = size;
        vm.run->io.count = 3;
        /* Unaligned packed data, ending exactly at the map boundary. */
        vm.run->io.data_offset = vm.run_size - 3 * size;
        uint8_t *data = (uint8_t *)vm.run + vm.run->io.data_offset;
        memset(data, 0x5a, 3 * size);
        assert(vm_handle_io(&vm) == 0 && calls == 3);
        uint32_t expected = size == 1 ? 0x5a : size == 2 ? 0x5a5a : 0x5a5a5a5a;
        for (unsigned i = 0; i < 3; i++)
            assert(values[i] == expected);
        calls = 0;
        vm.run->io.direction = KVM_EXIT_IO_IN;
        assert(vm_handle_io(&vm) == 0 && calls == 3);
        for (unsigned i = 0; i < 3; i++) {
            uint32_t value = 0x12345678U + i;
            assert(memcmp(data + i * size, &value, size) == 0);
        }
    }
    for (unsigned size = 0; size <= 255; size++) {
        if (size == 1 || size == 2 || size == 4)
            continue;
        prepare(&vm); vm.run->io.size = size; reject(&vm);
    }
    prepare(&vm); vm.run->io.count = 0; reject(&vm);
    prepare(&vm); vm.run->io.count = UINT32_MAX; reject(&vm);
    prepare(&vm); vm.run->io.size = 4; vm.run->io.count = UINT32_MAX; reject(&vm);
    prepare(&vm); vm.run->io.direction = 2; reject(&vm);
    prepare(&vm); vm.run->io.direction = 255; reject(&vm);
    prepare(&vm); vm.run->io.data_offset = UINT64_MAX; reject(&vm);
    prepare(&vm); vm.run->io.data_offset = vm.run_size; reject(&vm);
    prepare(&vm); vm.run->io.data_offset = vm.run_size + 1; reject(&vm);
    prepare(&vm); vm.run->io.data_offset = sizeof(struct kvm_run) - 1; reject(&vm);
    prepare(&vm); vm.run->io.data_offset = 0; reject(&vm);
    prepare(&vm); vm.run->io.count = 34; reject(&vm);
    prepare(&vm); vm.run->exit_reason = KVM_EXIT_HLT; reject(&vm);
    prepare(&vm); vm.run_size = sizeof(struct kvm_run) - 1; reject(&vm);
    assert(vm_run(&vm) == -1 && errno == EINVAL);
    vm.run_size = sizeof(struct kvm_run) + 33;
    prepare(&vm); accept_io = false;
    assert(vm_handle_io(&vm) == -1 && calls == 1);
    calls = 0; vm.run->io.direction = KVM_EXIT_IO_IN;
    assert(vm_handle_io(&vm) == -1 && calls == 1);
    accept_io = true;
    free(vm.run);
    vm.run = MAP_FAILED; assert(vm_handle_io(&vm) == -1);
    vm.run = NULL; assert(vm_handle_io(&vm) == -1);
    assert(vm_handle_io(NULL) == -1);
}

static void test_destroy(void) {
    struct vm vm = { .mem_size = 4096, .run_size = 4096 };
    vm.mem = mmap(NULL, vm.mem_size, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    vm.run = mmap(NULL, vm.run_size, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(vm.mem != MAP_FAILED && vm.run != MAP_FAILED);
    vm.kvm_fd = open("/dev/null", O_RDONLY);
    vm.vm_fd = open("/dev/null", O_RDONLY);
    vm.vcpu_fd = open("/dev/null", O_RDONLY);
    assert(vm.kvm_fd >= 0 && vm.vm_fd >= 0 && vm.vcpu_fd >= 0);
    vm_destroy(&vm);
    assert(vm.kvm_fd == -1 && vm.vm_fd == -1 && vm.vcpu_fd == -1);
    assert(vm.mem == MAP_FAILED && vm.run == MAP_FAILED);
    assert(vm.mem_size == 0 && vm.run_size == 0);
    int replacement = open("/dev/null", O_RDONLY);
    assert(replacement >= 0);
    vm_destroy(&vm);
    assert(fcntl(replacement, F_GETFD) >= 0);
    close(replacement);
    assert(vm_run(&vm) == -1);
    vm_destroy(NULL);
    assert(vm_create(&vm, 0) == -1 && errno == EINVAL);
    vm_destroy(&vm);
    assert(vm_create(&vm, 4097) == -1 && errno == EINVAL);
    vm_destroy(&vm);
    assert(vm_create(NULL, 4096) == -1 && errno == EINVAL);
}

int main(void) {
    test_io();
    test_destroy();
    puts("VM I/O and lifecycle tests passed");
    return 0;
}
