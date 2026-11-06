#pragma once
#include <stddef.h>
#include <stdint.h>

/* Kept opaque so callers don't need <linux/kvm.h>. */
struct kvm_run;

struct vm {
    int kvm_fd;              /* handle to /dev/kvm */
    int vm_fd;               /* the VM */
    int vcpu_fd;             /* the single vCPU */
    struct kvm_run *run;     /* shared vCPU state (mmap of vcpu_fd) */
    size_t run_size;
    uint8_t *mem;            /* guest RAM, guest-physical 0 maps here */
    size_t mem_size;
};

enum vm_run_result {
    VM_RUN_ERROR = -1,
    VM_RUN_HALTED = 0,
    VM_RUN_EXITED = 1,
};

/* Create a VM with `mem_size` bytes of guest RAM at guest-physical 0, one
 * vCPU, and a mapped kvm_run. mem_size must be nonzero and page-aligned.
 * Returns 0 on success, -1 on error (errno set); partial resources are cleaned
 * up on failure. Do not call on a live VM without first destroying it. */
int vm_create(struct vm *vm, size_t mem_size);

/* Copy `len` bytes of guest code to guest-physical `guest_addr`. */
int vm_load_guest(struct vm *vm, uint64_t guest_addr, const void *code, size_t len);

/* Put the vCPU in 16-bit real mode (flat, segment bases 0) with rip = entry. */
int vm_set_real_mode(struct vm *vm, uint64_t entry);

/* Build an identity-mapped low-memory paging hierarchy and GDT, then put the
 * vCPU directly into 64-bit long mode with rip = entry. */
int vm_set_long_mode(struct vm *vm, uint64_t entry);

/* Run until the guest halts, requests an MMIO-device exit, or fails. */
int vm_run(struct vm *vm);

/* Validate and service one KVM_EXIT_IO without running the vCPU. */
int vm_handle_io(struct vm *vm);

/* Safe after any vm_create attempt and on repeated calls; not on an
 * uninitialized or merely zero-initialized struct vm. */
void vm_destroy(struct vm *vm);
