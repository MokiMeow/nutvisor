# 04 — Roadmap

The path from "runs a real-mode guest" (today) to "boots a kernel guest." Each
milestone leaves a VMM that builds and runs a guest to completion, with a full
spec and a Definition of Done in [milestones/](milestones/).

## The plan

| # | Milestone | You'll build | You'll learn |
|---|-----------|--------------|--------------|
| 0 | **KVM hello** ✅ | VM/vCPU/memory, real-mode guest, serial + HLT | the KVM ioctl sequence, VM exits |
| 1 | **Serial** ✅ | a port-I/O dispatch table + fuller 16550 model | port I/O exits, device modelling |
| 2 | **Long mode** | GDT + page tables in guest memory, 64-bit sregs | real→long mode, guest paging |
| 3 | **MMIO** | `KVM_EXIT_MMIO` handling + a mapped device | memory-mapped I/O |
| 4 | **ELF loader** | parse ELF64, load segments, jump to entry | ELF, guest memory layout |
| 5 | **CPUID / robustness** | `KVM_SET_CPUID2`, every exit reason handled | CPUID, VM-exit completeness |
| 6 | **Polish** | self-test, CI, demo, tag `v1.0.0` | verification, presentation |

## Dependency order

```
M0 ─► M1 ─► M2 ─► M4 ─► M5 ─► M6
             └► M3 ──────┘
```

- **M1 before M2** — a solid serial channel is how you debug a long-mode guest.
- **M2 before M4** — a kernel guest runs in long mode, so bring long mode up
  first with a small purpose-built guest, then load a real image.
- **M3 (MMIO)** can be built in parallel with M2; M4 benefits from both.
- **M5** hardens whatever M4 boots (CPUID, clean exits) before the polish pass.

## Definition of Done (whole project)

nutvisor loads a 64-bit ELF guest kernel from a file, brings the vCPU up into
long mode, and runs that kernel so it produces output through the emulated
serial (and/or MMIO) device. CI builds on every push (and runs the guest where
`/dev/kvm` exists); the README has a demo; the release is tagged `v1.0.0`.

## Stretch goal (the headline)

**Boot the real Nutshell kernel as the guest.** Nutshell is a multiboot2 kernel
that expects the boot state GRUB provides (a magic value in `eax`, an info
pointer in `ebx`, 32-bit protected-mode entry). Booting it in nutvisor means
emulating that hand-off: load the ELF, build a minimal multiboot2 info structure
in guest memory, set the registers, and enter protected mode at the kernel's
entry point. This is the "I wrote the VM *and* the OS it runs" moment — ambitious,
optional, and documented as a stretch, not required for `v1.0.0`.

## Other stretch ideas (optional)

- A second vCPU (SMP) and inter-processor interrupts.
- A virtio-console or a tiny block device.
- Snapshot/restore of guest state.
- A `-gdb` mode exposing the guest to GDB.
