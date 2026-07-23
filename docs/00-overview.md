# 00 — Overview

## What nutvisor is

nutvisor is a type-2 hypervisor — a Virtual Machine Monitor (VMM) — written from
scratch in C. It uses the Linux **KVM** API to run guest code directly on the
CPU's hardware virtualization (Intel VT-x), and it emulates the small amount of
"hardware" the guest expects (a serial port, memory-mapped devices) in host
software. Its goal is to load and boot a small operating system as its guest.

## The one-sentence idea

> Create a virtual machine with the KVM API, run guest code on the real CPU, and
> service every VM exit the guest triggers — until the guest is a booting kernel.

## Type-2, using KVM — and why that's still deep

KVM does the part only the kernel can do (entering VT-x non-root mode, trapping
privileged operations). nutvisor does everything else, and that "everything
else" is the substance:

- deciding the guest's physical memory layout and installing it,
- setting the vCPU's initial register and segment state for a given CPU mode,
- loading guest code / a kernel image into guest memory,
- running the `KVM_RUN` loop and **handling each exit** — port I/O, MMIO, HLT,
  shutdown, and entry failures,
- emulating the devices behind those exits.

Writing a hypervisor entirely without KVM would mean implementing VT-x itself —
a different, far larger project that teaches CPU-vendor minutiae rather than how
virtualization is actually built in practice. See
[ADR 0001](decisions/0001-kvm-type2.md).

## Design goals

1. **Understandable end to end.** Every ioctl and every exit is explained.
2. **From scratch.** Only libc and `<linux/kvm.h>` — no VMM libraries.
3. **Always runnable.** Every milestone leaves a VMM that builds and runs a
   guest to completion.
4. **Zero cost.** Runs locally on WSL2/Linux with free tools; needs only
   `/dev/kvm` (nested virtualization, which Windows 11 + WSL2 provides).

## What it is *not* (v1)

- Not multi-vCPU or SMP (one vCPU).
- Not a full device model (no PCI, no disk, no network in v1).
- Not a replacement for QEMU/kvmtool — it's a teaching-grade VMM.

## The shape of the system

```
 nutvisor (host process)
   ├─ vm.c        open /dev/kvm, KVM_CREATE_VM, guest memory, vCPU, KVM_RUN loop
   ├─ serial.c    COM1 device: guest port I/O -> host stdout        (M0, M1)
   ├─ mmio.c      memory-mapped device(s)                           (M3)
   ├─ loader.c    ELF64 image -> guest memory                       (M4)
   └─ main.c      load a guest, choose its CPU mode, run it
        │
        ▼
   guest: real-mode blob (M0) -> long-mode program (M2) -> a kernel (M4+)
```

Read the [architecture doc](02-architecture.md) next, or jump to
[getting started](01-getting-started.md) to run it.
