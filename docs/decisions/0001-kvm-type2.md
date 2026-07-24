# ADR 0001 — Build a type-2 hypervisor on KVM (not VT-x from scratch)

**Status:** accepted · **Date:** 2026

## Context

To run a guest on real hardware virtualization we need to enter VT-x non-root
mode, which is a privileged (ring-0) operation. Options:

1. Write a kernel module / bare-metal code that programs VT-x directly (VMCS,
   VMLAUNCH/VMRESUME, EPT).
2. Use the Linux **KVM** API (`/dev/kvm`) from a normal user-space process.

## Decision

Build a **type-2 VMM on the KVM API**.

## Rationale

- KVM does exactly the part that *must* live in the kernel (VMCS management,
  non-root entry), and exposes everything else — memory, vCPU registers, the
  exit loop, device emulation — to user space. That "everything else" is the
  substance of building a hypervisor and is where the learning is.
- A from-scratch VT-x implementation is a kernel-development project about one
  vendor's virtualization ISA (VMCS field encodings, EPT paging), not about how
  hypervisors are actually built and run today.
- KVM is how real VMMs (QEMU, Firecracker, kvmtool, crosvm) work, so the skills
  transfer directly.

## Consequences

- Requires `/dev/kvm` and nested virtualization (fine on Linux and Windows 11 +
  WSL2; see [docs/01](../01-getting-started.md)).
- We depend on the Linux KVM ABI (`<linux/kvm.h>`), and nothing else.
- CI **builds** everywhere and **runs the guest wherever `/dev/kvm` is usable**.
  GitHub's Ubuntu runners do provide the device but root-only, so the workflow
  installs a udev rule to open it before running; it degrades to a skip (not a
  failure) only if the device is genuinely unusable.
