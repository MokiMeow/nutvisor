# 05 — Guest memory

*Introduced in milestone 0; deepened in milestones 2 and 4.*

## The core idea

The guest's physical RAM is just host memory you `mmap` and hand to KVM:

```c
uint8_t *mem = mmap(NULL, size, PROT_READ|PROT_WRITE,
                    MAP_SHARED|MAP_ANONYMOUS, -1, 0);
struct kvm_userspace_memory_region region = {
    .slot = 0,
    .guest_phys_addr = 0,          // where it appears in the guest
    .memory_size = size,
    .userspace_addr = (uint64_t)mem // where it lives in the host
};
ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &region);
```

After this, **guest-physical address `P` is `mem + P`** on the host. Loading a
guest is a `memcpy` into `mem` at the right offset; inspecting guest memory
after a run is a read from `mem`.

## Memory slots

A VM has several numbered *slots*, each mapping a host range to a guest-physical
range. v1 uses one slot for all of low RAM. You'd add slots for:
- a separate region for a loaded kernel at a high address,
- read-only regions (`KVM_MEM_READONLY`) to trap writes,
- holes that deliberately aren't backed, so accesses become `KVM_EXIT_MMIO`
  (that's how MMIO devices work — see [docs/07](07-device-emulation.md)).

## Layout as the guest grows

| Milestone | Guest layout |
|-----------|--------------|
| M0 | 1 MiB flat; guest blob at `0x1000`, real mode |
| M2 | + a GDT and page tables placed in guest memory for long mode |
| M4 | ELF `PT_LOAD` segments copied to their `p_paddr`; entry at `e_entry` |

## Gotchas

- **Bounds-check every load.** `guest_addr + len` must fit in `mem_size`
  (nutvisor's `vm_load_guest` does this) — a guest image that overruns silently
  corrupts adjacent guest memory.
- **Alignment.** Page tables and some structures the guest expects must be
  page-aligned in guest-physical space; choose load addresses accordingly.
- **Identity vs. higher-half.** For long mode you decide the guest's virtual→
  physical mapping by writing its page tables (M2); the simplest is an identity
  map of low memory.
