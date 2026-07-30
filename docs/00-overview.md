# 00: Overview

## What nutvisor is

nutvisor is a type-2 hypervisor: a Virtual Machine Monitor (VMM): written from
scratch in C. It uses the Linux **KVM** API to run guest code directly on the
CPU's hardware virtualization (Intel VT-x), and it emulates the small amount of
"hardware" the guest expects (a serial port, memory-mapped devices) in host
software. The v1 release loads and boots a purpose-built ELF64 kernel.

## The one-sentence idea

> Create a virtual machine with the KVM API, run guest code on the real CPU, and
> service every VM exit the guest triggers while an ELF64 kernel boots.

## Type-2, using KVM, and why that is still deep

KVM does the part only the kernel can do (entering VT-x non-root mode, trapping
privileged operations). nutvisor does everything else, and that "everything
else" is the substance:

- deciding the guest's physical memory layout and installing it,
- setting the vCPU's initial register and segment state for a given CPU mode,
- loading guest code / a kernel image into guest memory,
- running the `KVM_RUN` loop and **handling each exit**: port I/O, MMIO, HLT,
  shutdown, and entry failures,
- emulating the devices behind those exits.

Writing a hypervisor entirely without KVM would mean implementing VT-x itself,
a different, far larger project that teaches CPU-vendor minutiae rather than how
virtualization is actually built in practice. See
[ADR 0001](decisions/0001-kvm-type2.md).

## Design goals

1. **Understandable end to end.** Every ioctl and every exit is explained.
2. **From scratch.** Only libc and `<linux/kvm.h>`: no VMM libraries.
3. **Always runnable.** Every milestone leaves a VMM that builds and runs a
   guest to completion.
4. **Zero cost.** Runs locally on WSL2/Linux with free tools; needs only
   `/dev/kvm` (nested virtualization, which Windows 11 + WSL2 provides).

## What it is *not* (v1)

- Not multi-vCPU or SMP (one vCPU).
- Not a full device model (no PCI, no disk, no network in v1).
- Not a replacement for QEMU/kvmtool: it's a teaching-grade VMM.

## The shape of the system

```
 nutvisor (host process)
   ├─ vm.c        KVM lifecycle, CPU modes, exit loop, diagnostics
   ├─ cpuid.c     KVM-supported CPUID table -> vCPU
   ├─ ioport.c    port-range dispatch -> serial.c (16550 COM1)
   ├─ mmio.c      memory-mapped debug console + exit device
   ├─ loader.c    validated ELF64 PT_LOAD segments -> guest memory
   └─ main.c      choose image format / CPU mode and orchestrate
        │
        ▼
   guest: regression blobs + the ELF64 kernel in guests/kernel/
```

Read the [architecture doc](02-architecture.md) next, or jump to
[getting started](01-getting-started.md) to run it.
