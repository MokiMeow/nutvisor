# Changelog

All notable changes to nutvisor are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project aims
to follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Milestone 5: dynamically sized `KVM_GET_SUPPORTED_CPUID` /
  `KVM_SET_CPUID2` setup, exercised by the ELF guest before its boot marker.
- Complete VM-exit reporting with automatic `rip`/segment/control-register
  dumps, plus a deliberate `ud2` guest for triple-fault diagnostics.
- Milestone 4: a two-pass ELF64 loader that validates headers, segment/file/RAM
  bounds, executable entry placement, and `.bss` sizing before modifying guest
  RAM, then loads every `PT_LOAD` segment and zero-fills it.
- An ELF64 guest kernel linked at `0x100000`; its runtime `.bss` assertion and
  `nutvisor: elf64 kernel online` marker exercise the loader end to end.
- Milestone 3: generic `KVM_EXIT_MMIO` dispatch plus a control device at
  `0x10000000` with a debug console and status-code exit register.
- A 64-bit MMIO demo guest that prints `nutvisor: mmio console online` and
  requests a clean device exit.
- Milestone 2: direct 64-bit long-mode vCPU entry with an in-guest GDT, a
  four-level identity map of the low GiB using 2 MiB pages, and a valid stack.
- A 64-bit flat-binary guest that prints `nutvisor: long mode online` through
  the emulated UART and halts.
- Milestone 1: a reusable port-I/O dispatch table and a stateful 16550 COM1
  model with DLAB/divisor, line-control, interrupt, FIFO, modem-control, line
  status, and scratch-register behaviour.
- A driver-style real-mode guest that configures COM1, polls transmitter
  readiness, prints `nutvisor: 16550 driver online`, and halts.
- Milestone 0: KVM bring-up — open `/dev/kvm`, create a VM, guest memory, and a
  vCPU; put the vCPU in real mode; run a 16-bit guest that prints to COM1 and
  halts, servicing `KVM_EXIT_IO` and `KVM_EXIT_HLT`.
- A minimal COM1 serial device model on the host side.
- Build system: `Makefile` (`all`/`run`/`check-kvm`/`clean`) with automatic KVM
  module loading, and a `setup-kvm.sh` helper.
- GitHub Actions CI that builds the VMM and guest, and runs milestone 0 where
  `/dev/kvm` is available (skipping the run, not the build, otherwise).
- Documentation set under `docs/` and the `AGENTS.md` operating manual.

## [0.1.0] — milestone 0
- First working version: runs a real-mode guest inside a KVM VM and prints its
  output on the host.
