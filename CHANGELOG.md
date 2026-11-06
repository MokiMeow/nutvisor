# Changelog

All notable changes to nutvisor are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project aims
to follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Hardware-free `--check` preflight, loader/device/I/O/lifecycle regression tests,
  and separate host/ASan/UBSan test targets that run even without KVM.
- A documented bootstrap memory contract and security boundary/research rationale.

### Fixed
- Reject ELF segments that overlap bootstrap tables/stack, overlap one another,
  use unsupported virtual layouts/dynamic linking, or violate alignment rules.
- Require an entry in file-backed executable bytes and validate before opening
  KVM; cap input file allocation at 128 MiB.
- Validate complete packed KVM I/O buffers before dispatch, including widths,
  directions, counts and overflow-safe offsets.
- Clean up partial VM creation, reject undersized run mappings and make repeated
  destruction safe without closing subsequently reused descriptors.
- Track header dependencies and honor linker flags in incremental builds.
- Reject timeout/signal exits in deliberate-failure guest tests instead of
  mistaking them for expected errors.

### Changed
- Expanded the README with explicit limitations and a complete documentation
  map, and standardized first-party Markdown punctuation.

## [1.0.0]: 2026-07-25

### Added
- Milestone 6: an end-to-end `make test` suite covering every guest, MMIO
  status exits, malformed ELF rejection, and triple-fault diagnostics.
- CI execution of the complete KVM suite with a generous cold-start timeout,
  plus an asciinema recording, embedded terminal preview, and VM-exit flow
  diagram.
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
- Milestone 0: KVM bring-up: open `/dev/kvm`, create a VM, guest memory, and a
  vCPU; put the vCPU in real mode; run a 16-bit guest that prints to COM1 and
  halts, servicing `KVM_EXIT_IO` and `KVM_EXIT_HLT`.
- A minimal COM1 serial device model on the host side.
- Build system: `Makefile` (`all`/`test`/`run`/`check-kvm`/`clean`) with automatic KVM
  module loading, and a `setup-kvm.sh` helper.
- GitHub Actions build infrastructure with conditional `/dev/kvm` access.
- Documentation set under `docs/` and the `AGENTS.md` operating manual.

## [0.1.0]: milestone 0
- First working version: runs a real-mode guest inside a KVM VM and prints its
  output on the host.
