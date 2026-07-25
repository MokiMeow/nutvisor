# Milestone 2 — Long-mode guest ✅ (done)

**Goal:** bring the vCPU up into 64-bit long mode and run a 64-bit guest that
prints via serial. This is the gateway to booting a real kernel.

## Concepts

The long-mode entry requirements (`cr0.PG`, `cr4.PAE`, `efer.LME/LMA`, a 64-bit
code descriptor, valid `cr3` page tables), and building a GDT + page tables
*in guest memory* from the host.

## Tasks

- [x] In guest memory, build an identity-mapped page table for the low region:
      one PML4 → one PDPT → one PD of 2 MiB pages (covers 1 GiB). Place them
      page-aligned at known guest-physical addresses.
- [x] Build a minimal GDT in guest memory (null, 64-bit code, data) — or set the
      `cs`/`ds` descriptors directly in `sregs`.
- [x] Add `vm_set_long_mode(entry)`: set `sregs` — `cr3` = page-table base,
      `cr4.PAE`, `cr0.PG|PE`, `efer.LME|LMA`, a 64-bit `cs` (L=1), flat data
      segments — then `KVM_SET_SREGS`; set `rip = entry`, a valid `rsp`.
- [x] Add a 64-bit guest (`guests/hello64.asm`, `nasm -f bin`, `bits 64`) that
      writes a string to `0x3F8` and halts.
- [x] Load the guest at its expected physical address and run it.

## Files

`src/vm.c` + `include/vmm.h` (`vm_set_long_mode`, page-table/GDT builder — or a
new `src/paging.c`), `guests/hello64.asm`, `src/main.c` (mode selection).

## Definition of Done

- [x] `make run` with the 64-bit guest prints its message and halts cleanly —
      no `KVM_EXIT_SHUTDOWN`/`FAIL_ENTRY`.
- [x] A register dump (or a comment) documents the exact `cr0/cr4/efer/cs` bits
      set, so the transition is reproducible.
- [x] `make all` clean; earlier guests still run.

## Notes

`FAIL_ENTRY` here almost always means an inconsistent `sregs` (e.g. `LMA` set
without `PG`, or `cr3` not pointing at valid tables). Dump `sregs` before
`KVM_RUN` and check each bit (see [docs/09](../09-testing-and-debugging.md)).

## References

- OSDev Wiki — [Setting Up Long Mode](https://wiki.osdev.org/Setting_Up_Long_Mode)
- [docs/06 — vCPU & CPU modes](../06-vcpu-and-modes.md)

**Next:** [Milestone 3 — MMIO](milestone-3-mmio.md).
