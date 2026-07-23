# 06 — vCPU & CPU modes

*Real mode is milestone 0; long mode is milestone 2.*

The vCPU's state is two structs: `kvm_regs` (GP registers, `rip`, `rsp`,
`rflags`) and `kvm_sregs` (segments + control registers `cr0/cr3/cr4/efer`).
Which CPU mode the guest runs in is entirely a function of those special
registers — and *you* set them from the host.

## Real mode (milestone 0)

The simplest mode: 20-bit addressing, segment×16+offset. After
`KVM_CREATE_VCPU` the vCPU resets like a real CPU (with `cs.base` at the top of
memory). nutvisor rebases the code segment so a flat, low `rip` runs guest-
physical code directly:

```c
ioctl(vcpu, KVM_GET_SREGS, &sregs);
sregs.cs.base = 0;
sregs.cs.selector = 0;
ioctl(vcpu, KVM_SET_SREGS, &sregs);
struct kvm_regs regs = { .rip = 0x1000, .rflags = 0x2 };
ioctl(vcpu, KVM_SET_REGS, &regs);
```

That's all it takes to run 16-bit code — which is why milestone 0 fits in a page
of C.

## Long mode (milestone 2) — the real work

64-bit mode requires, before entry: paging on (`cr0.PG`), PAE (`cr4.PAE`), the
long-mode-enable bit (`efer.LME`/`LMA`), a 64-bit code segment in the GDT, and
valid page tables in `cr3`. Two ways to get there:

1. **Host sets long mode directly.** Build a GDT and a 4-level page table
   *in guest memory*, point `sregs.cr3` at the page tables, set `cr0/cr4/efer`
   and a 64-bit `cs` descriptor, then set `rip` to the 64-bit entry. The guest
   starts already in long mode. (More host code; the guest can be trivial.)
2. **Guest bootstraps itself.** Start the guest in real/protected mode with a
   small assembly stub that does the same paging + mode switch (exactly what a
   real kernel's boot code does), then jumps to 64-bit. (Less host code; the
   guest carries the transition — like Nutshell's `boot.asm`.)

Milestone 2 recommends **option 1** first (it teaches the sregs/paging setup
that's the point of this project), with option 2 available for kernel guests
that already bootstrap themselves.

The identity-mapped page table for the low region is small: one PML4 entry → one
PDPT entry → one PD full of 2 MiB pages covers the first gigabyte. Place these
page-aligned in guest memory and set `cr3` to their guest-physical address.

## Protected mode (32-bit)

A useful intermediate (and what a multiboot kernel entry expects): `cr0.PE` set,
paging off or on, a 32-bit `cs`. Relevant to the Nutshell stretch goal, where
the guest enters in 32-bit protected mode.

## Debugging mode transitions

If a mode switch is wrong the guest usually faults immediately →
`KVM_EXIT_SHUTDOWN` (triple fault) or `KVM_EXIT_FAIL_ENTRY`. Dump `sregs`/`regs`
with `KVM_GET_*` right before `KVM_RUN` and check each bit against the manual
(see [docs/09](09-testing-and-debugging.md)).
