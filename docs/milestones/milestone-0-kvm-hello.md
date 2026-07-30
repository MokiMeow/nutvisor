# Milestone 0: KVM hello ✅ (done)

**Goal:** create a KVM virtual machine and run a real-mode guest that prints to
serial and halts.

## Concepts

The KVM ioctl sequence, guest memory as a host `mmap`, the vCPU and its
`kvm_run` structure, the `KVM_RUN` exit loop, real-mode vCPU setup, and turning
`KVM_EXIT_IO`/`KVM_EXIT_HLT` into host behaviour.

## What shipped

- [x] `vm_create`: open `/dev/kvm`, check API version, `KVM_CREATE_VM`, `mmap`
      guest RAM + `KVM_SET_USER_MEMORY_REGION`, `KVM_CREATE_VCPU`, `mmap` the
      `kvm_run`. (`src/vm.c`, `include/vmm.h`)
- [x] `vm_set_real_mode`: rebase `cs` to 0 and set `rip`/`rflags` for flat
      real-mode execution.
- [x] `vm_run`: the exit loop handling `KVM_EXIT_IO` (serial), `KVM_EXIT_HLT`
      (done), and reporting `SHUTDOWN`/`FAIL_ENTRY`/`INTERNAL_ERROR`.
- [x] `src/serial.c`: COM1 model turning port `0x3F8` writes into stdout.
- [x] `guests/hello16.asm`: a 16-bit guest that prints a string and halts.
- [x] `src/main.c`: read a guest image from a file, load, run.
- [x] Build system with automatic KVM-module loading; CI that builds and runs
      where `/dev/kvm` exists.

## Definition of Done

- [x] `make all` builds with no warnings (`-std=gnu11`).
- [x] `make run` prints the guest's line: `nutvisor: the guest is alive inside
      your hypervisor`, then `guest halted cleanly`, and exits 0.

## References

- LWN: [Using the KVM API](https://lwn.net/Articles/658511/)
- Linux: [KVM API docs](https://www.kernel.org/doc/html/latest/virt/kvm/api.html)

**Next:** [Milestone 1: Serial](milestone-1-serial.md).
