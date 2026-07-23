# 02 — Architecture

nutvisor is a single host process that owns one VM with one vCPU. The design is
a thin lifecycle layer around KVM plus a set of device models behind the VM-exit
loop.

## Components

```
┌──────────────────────────────────────────────┐
│ main.c   choose guest + CPU mode, orchestrate │
├──────────────────────────────────────────────┤
│ loader   ELF64 image -> guest memory     (M4) │
├──────────────────────────────────────────────┤
│ vm.c     KVM lifecycle + the KVM_RUN loop     │
│          (create, memory, vcpu, exits)        │
├───────────────┬──────────────┬────────────────┤
│ serial.c (M0) │ mmio.c (M3)  │ cpuid (M5)     │  device / exit handlers
├───────────────┴──────────────┴────────────────┤
│ /dev/kvm  (kernel: VT-x non-root execution)   │
└──────────────────────────────────────────────┘
```

Lower layers never call up. `vm.c` dispatches exits to device handlers; device
handlers never touch KVM directly.

## The central object

```c
struct vm {
    int kvm_fd;              // /dev/kvm
    int vm_fd;               // the VM
    int vcpu_fd;             // the vCPU
    struct kvm_run *run;     // shared vCPU state (mmap of vcpu_fd)
    uint8_t *mem;            // guest RAM; guest-physical 0 maps to mem[0]
    size_t   mem_size;
};
```

`mem` is ordinary host memory handed to the guest as its physical RAM via
`KVM_SET_USER_MEMORY_REGION`. Guest-physical address `P` is `mem + P`. `run` is
a shared page the kernel fills in on each exit (why it exited, the port, the
MMIO address, etc.).

## Control flow

```
main: read guest image
      vm_create()           open /dev/kvm, KVM_CREATE_VM, map memory + vcpu
      vm_load_guest()       copy the image into guest memory
      set CPU mode          real mode (M0) / long mode (M2) / from ELF (M4)
      vm_run():
          loop:
              ioctl(KVM_RUN)                  // hand the CPU to the guest
              switch (run->exit_reason):      // guest trapped back to us
                  IO       -> serial / device handler
                  MMIO     -> mmio handler        (M3)
                  HLT      -> return (done)
                  SHUTDOWN / FAIL_ENTRY -> report and stop
```

The guest runs at native speed until it does something that must trap to the
host (an I/O instruction, an access to unmapped memory, a halt); KVM stops the
guest, fills `run`, and returns from `KVM_RUN`. We service it and loop.

## Device model

Devices are plain host code behind exits:

- **Port I/O devices** (serial): a `KVM_EXIT_IO` carries a port, direction,
  size, count, and a data offset into `run`. The handler reads/writes those
  bytes. (M0/M1)
- **MMIO devices**: a `KVM_EXIT_MMIO` carries a physical address, a length, a
  read/write flag, and a data buffer. (M3)

Each device declares which addresses it owns; `vm.c` routes exits to the owner.

## Source map

| Path | Role | Added in |
|------|------|----------|
| `src/vm.c`, `include/vmm.h` | KVM lifecycle + exit loop | M0 |
| `src/serial.c`, `include/serial.h` | COM1 device | M0/M1 |
| `src/main.c` | load a guest, pick mode, run | M0 |
| `guests/hello16.asm` | real-mode demo guest | M0 |
| `src/mmio.c` | memory-mapped device(s) | M3 |
| `src/loader.c` | ELF64 loader | M4 |
| `guests/*` | long-mode + kernel guests | M2, M4 |

See the [roadmap](04-roadmap.md) for the order and the
[milestones](milestones/) for the detail of each.
