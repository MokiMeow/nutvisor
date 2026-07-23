# ADR 0003 — Reliable purpose-built guests first, Nutshell as a stretch

**Status:** accepted · **Date:** 2026

## Context

The headline goal is "boot the Nutshell kernel as the guest." But Nutshell is a
multiboot2 kernel: it expects the exact boot state GRUB provides (magic in
`eax`, an info structure pointer in `ebx`, 32-bit protected-mode entry). Booting
it directly means emulating a chunk of GRUB and the BIOS hand-off.

## Decision

Build the milestones around **purpose-written guests** (a real-mode blob, a
long-mode program, then a small 64-bit ELF guest kernel). Make **booting the
real Nutshell kernel a documented stretch goal**, not a required milestone.

## Rationale

- Each milestone should leave a VMM that *provably runs a guest to completion*.
  Purpose-built guests let us verify one VMM capability at a time (serial, long
  mode, MMIO, ELF loading) without also debugging a full kernel's expectations.
- The "I wrote the VM and the OS it runs" story is satisfied by a 64-bit ELF
  guest kernel we write — Nutshell-the-multiboot-kernel is a bonus, not the bar.
- Multiboot2 emulation (building the info structure, protected-mode entry) is a
  meaningful project in itself; scoping it as a stretch keeps `v1.0.0`
  achievable and honest.

## Consequences

- The guests live in `guests/`, built alongside the VMM.
- Milestone 4 boots a 64-bit ELF guest kernel; milestone 6's stretch attempts
  Nutshell via multiboot2 emulation and documents the result either way.
