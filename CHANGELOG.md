# Changelog

All notable changes to nutvisor are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project aims
to follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
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
