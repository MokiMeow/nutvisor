# 03: The KVM API

This is the API the whole project is built on. It is a set of `ioctl`s on three
file descriptors: the KVM subsystem, a VM, and a vCPU. This doc explains the
complete bring-up and exit path used by nutvisor.

## The three file descriptors

```
open("/dev/kvm")  ── the KVM subsystem (system-wide)
      │  ioctl KVM_CREATE_VM
      ▼
   vm_fd          ── one virtual machine (owns guest memory)
      │  ioctl KVM_CREATE_VCPU
      ▼
   vcpu_fd        ── one virtual CPU (owns register state)
```

## Complete bring-up sequence

1. **`open("/dev/kvm", O_RDWR)`**: the handle to KVM.
2. **`ioctl(kvm, KVM_GET_API_VERSION)`**: must be `KVM_API_VERSION` (12); a
   sanity check that the ABI matches.
3. **`ioctl(kvm, KVM_CREATE_VM)`** → `vm_fd`.
4. **Guest memory**: `mmap` an anonymous region on the host, then
   `ioctl(vm, KVM_SET_USER_MEMORY_REGION, &region)` to map it at a chosen
   guest-physical address (0 for us). The guest's RAM *is* that host mapping.
5. **`ioctl(vm, KVM_CREATE_VCPU)`** → `vcpu_fd`.
6. **Guest CPUID**: dynamically fetch `KVM_GET_SUPPORTED_CPUID` from the KVM
   fd and install the complete table with `KVM_SET_CPUID2` on the vCPU.
7. **The shared run structure**: `ioctl(kvm, KVM_GET_VCPU_MMAP_SIZE)` then
   `mmap(vcpu_fd)` gives a `struct kvm_run` the kernel updates on every exit.
8. **Initial CPU state**: `KVM_GET_SREGS`/`KVM_SET_SREGS` for segments and
   control registers, `KVM_SET_REGS` for `rip`, `rsp`, `rflags`, GP registers.
9. **Run**: `ioctl(vcpu, KVM_RUN)` in a loop, dispatching on
   `run->exit_reason`.

## The VM-exit loop

`KVM_RUN` returns when the guest does something the host must handle. `run`
tells you what:

| `exit_reason` | Meaning | Handled in |
|---------------|---------|------------|
| `KVM_EXIT_IO` | guest executed `in`/`out` | M0 (serial), M1 |
| `KVM_EXIT_HLT` | guest executed `hlt` | M0 |
| `KVM_EXIT_MMIO` | guest read/wrote unmapped phys memory | M3 |
| `KVM_EXIT_SHUTDOWN` | triple fault / reset | reported |
| `KVM_EXIT_FAIL_ENTRY` | CPU couldn't enter the guest | reported |
| `KVM_EXIT_INTERNAL_ERROR` | KVM couldn't emulate something | reported |
| `KVM_EXIT_DEBUG` | guest debug/single-step exit | reported with state |
| `KVM_EXIT_EXCEPTION` | userspace-visible exception | reported with state |

All other values are explicitly reported by number and accompanied by a
register/control-state dump. Nothing falls through silently.

### Reading an I/O exit

For `KVM_EXIT_IO`, `run->io` gives `direction` (in/out), `port`, `size` (bytes
per access), `count` (how many), and `data_offset` (where the bytes live inside
the `run` mapping):

```c
uint8_t *data = (uint8_t *)run + run->io.data_offset;
for (i = 0; i < run->io.count; i++)
    handle(run->io.port, run->io.direction, data + i * run->io.size);
```

### Reading an MMIO exit (M3)

`run->mmio` gives `phys_addr`, `len`, `is_write`, and a `data[8]` buffer: on a
write the guest's bytes are in `data`; on a read you fill `data` with the
device's response.

## Registers: two structs

- **`kvm_regs`**: the general-purpose registers, `rip`, `rsp`, `rflags`.
- **`kvm_sregs`**: the "special" registers: segment descriptors
  (`cs`, `ds`, ...), and control registers `cr0/cr2/cr3/cr4`, `efer`. This is
  what you change to move the guest between real, protected, and long mode
  (see [docs/06](06-vcpu-and-modes.md)).

## Things that bite

- **Real mode entry**: after `KVM_CREATE_VCPU` the vCPU resets with `cs.base`
  high; rebase `cs.base = 0` (and `cs.selector = 0`) to run flat code at a low
  `rip`. nutvisor does exactly this in `vm_set_real_mode`.
- **`KVM_SET_USER_MEMORY_REGION` order**: set memory *before* running; changing
  regions while running needs care.
- **Always check `ioctl` return values**: a `-1` with `errno` is how almost
  every VMM bug announces itself.

## References

- LWN: [Using the KVM API](https://lwn.net/Articles/658511/)
- Linux: [Documentation/virt/kvm/api.rst](https://www.kernel.org/doc/html/latest/virt/kvm/api.html)
