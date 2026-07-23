<h1 align="center">nutvisor</h1>

<p align="center">
  <em>A type-2 hypervisor written from scratch in C, using the Linux KVM API to
  run a guest on real hardware virtualization — with the goal of booting a small
  operating system as its guest.</em>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/arch-x86__64-blue" alt="x86_64">
  <img src="https://img.shields.io/badge/tech-KVM%20%2F%20VT--x-red" alt="KVM">
  <img src="https://img.shields.io/badge/lang-C%20%2B%20Assembly-orange" alt="C + Assembly">
  <img src="https://img.shields.io/badge/license-MIT-lightgrey" alt="MIT">
</p>

---

## What this is

nutvisor is a Virtual Machine Monitor (VMM): the program that *is* the "VM" when
you run one. It asks the CPU — via Intel VT-x, exposed through Linux's `/dev/kvm`
— to run guest code directly on the silicon, and it services every **VM exit**
the guest triggers: port I/O, memory-mapped I/O, halts, and faults. It creates
the guest's memory, sets up its virtual CPU, loads guest code, and emulates the
devices the guest talks to (starting with a serial port).

It is the companion to [Nutshell](../nutshell) — the from-scratch kernel — and
its endgame is to **boot that kernel as its guest**: *"I wrote the VM and the OS
it runs."*

```
   host (Linux/WSL2)                         guest
 +-------------------+   ioctl(KVM_RUN)   +--------------+
 |     nutvisor      | -----------------> |  guest code  |
 |  - guest memory   |                    |  (real mode, |
 |  - vCPU setup     |   <-- VM exit ---  |   long mode, |
 |  - device model   |   I/O, MMIO, HLT   |   a kernel)  |
 +-------------------+                    +--------------+
        /dev/kvm  <---- VT-x ----> physical CPU
```

## Why it is interesting (the depth on show)

- **The KVM API, by hand** — `KVM_CREATE_VM`, guest memory regions, `KVM_CREATE_VCPU`,
  the shared `kvm_run` structure, and the `KVM_RUN` exit loop.
  ([docs/03-kvm-api.md](docs/03-kvm-api.md))
- **VM exits** — turning a guest's `out dx, al` into a character, its MMIO into a
  device access, its `hlt` into a clean stop, and its faults into readable
  diagnostics. ([docs/07-device-emulation.md](docs/07-device-emulation.md))
- **CPU modes from the outside** — putting a vCPU into real mode, then building
  the GDT and page tables to bring a guest up into 64-bit long mode.
  ([docs/06-vcpu-and-modes.md](docs/06-vcpu-and-modes.md))
- **Loading a real kernel** — parsing an ELF64 image into guest-physical memory
  and jumping to its entry point. ([docs/08-elf-loading.md](docs/08-elf-loading.md))

## Quick start (WSL2 / Linux with nested virtualization, $0)

```bash
./scripts/setup-kvm.sh   # ensure /dev/kvm is available (loads the module)
make run                 # build the VMM + guest and run milestone 0
```

Expected output:

```
nutvisor: running build/hello16.bin (68 bytes) as a real-mode guest
nutvisor: the guest is alive inside your hypervisor
nutvisor: guest halted cleanly
```

That line came *from inside a VM your program created*. See
[docs/01-getting-started.md](docs/01-getting-started.md) for the nested-virt
requirements (Windows 11 + WSL2 exposes VT-x by default).

## Status

Milestone 0 (KVM bring-up + a real-mode guest that prints and halts) is **done**
— the repo builds and runs today. The road to booting a full kernel guest is
tracked in [docs/04-roadmap.md](docs/04-roadmap.md).

| # | Milestone | State |
|---|-----------|-------|
| 0 | KVM bring-up + real-mode "hello" guest | ✅ done |
| 1 | Serial device model + port-I/O dispatch | ⬜ |
| 2 | 64-bit long-mode guest (GDT + paging setup) | ⬜ |
| 3 | Memory-mapped I/O device emulation | ⬜ |
| 4 | ELF64 guest loader (boot a kernel from a file) | ⬜ |
| 5 | CPUID filtering + robust exit handling | ⬜ |
| 6 | Tests, CI, demo, `v1.0.0` (stretch: boot Nutshell) | ⬜ |

## Repository layout

```
nutvisor/
├── src/          # the VMM (C, auto-discovered by the Makefile)
├── include/      # VMM headers
├── guests/       # guest programs the VMM runs (asm/C)
├── scripts/      # setup-kvm.sh and helpers
├── docs/         # KVM API, architecture, roadmap, milestones, ADRs
└── Makefile      # all / run / check-kvm / clean
```

## Requirements

A Linux host (or Windows 11 + WSL2) with **nested virtualization** and
`/dev/kvm`. `make run` loads the KVM module automatically if it is missing; see
[docs/01-getting-started.md](docs/01-getting-started.md) if `/dev/kvm` cannot be
created.

## License

MIT — see [LICENSE](LICENSE).
