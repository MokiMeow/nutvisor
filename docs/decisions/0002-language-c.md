# ADR 0002: Write the VMM in C (gnu11)

**Status:** accepted · **Date:** 2026

## Context

The VMM is a user-space program that makes `ioctl`s on `/dev/kvm`, manages
memory with `mmap`, and manipulates C structs defined by `<linux/kvm.h>`.

## Decision

Write it in **C**, compiled with **`-std=gnu11`**.

## Rationale

- The KVM API is a C API. Every reference (the kernel docs, LWN's "Using the KVM
  API", QEMU/kvmtool source) is in C, so the mapping from documentation to code
  is direct.
- The work is struct-filling and syscall-checking, C's natural territory,
  which keeps the VMM small and the KVM interaction unmediated.
- `-std=gnu11` (not strict `-std=c11`) is required so POSIX/Linux symbols like
  `O_CLOEXEC` and `MAP_ANONYMOUS` are declared without hand-rolling feature-test
  macros. This was a real early build failure; the flag choice is deliberate.
- (Portfolio context: this pairs with the C/assembly Nutshell kernel, and keeps
  the systems-language story consistent.)

## Consequences

- No memory-safety net; discipline is checking every `ioctl`/`mmap`/`open`
  return value (see [AGENTS.md](../../AGENTS.md) §4) and reporting the exit
  reason on failure.
- Guest blobs are NASM (`-f bin` for real mode; ELF for kernel guests).
