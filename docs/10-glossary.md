# 10 — Glossary

- **Type-2 hypervisor** — a VMM that runs on a host OS and uses that OS's
  virtualization support (here, Linux KVM). Contrast with type-1 (bare metal).
- **VMM (Virtual Machine Monitor)** — the program that creates and runs a VM;
  nutvisor is one.
- **KVM** — Kernel-based Virtual Machine, the Linux subsystem exposing hardware
  virtualization through `/dev/kvm`.
- **VT-x / AMD-V** — Intel/AMD CPU hardware-virtualization extensions KVM drives.
- **Nested virtualization** — running a hypervisor inside a VM (WSL2 is itself a
  VM, so KVM inside it is nested). Required for nutvisor on WSL2.
- **vCPU** — a virtual CPU; one `vcpu_fd`, one set of register state.
- **VM exit** — the CPU leaving guest execution back to the host because the
  guest did something that must be handled (I/O, MMIO, HLT, fault).
- **`KVM_RUN`** — the ioctl that hands the CPU to the guest; it returns on a VM
  exit.
- **`kvm_run`** — the shared structure (mmap of the vcpu fd) the kernel fills
  with the exit reason and its details.
- **`kvm_regs` / `kvm_sregs`** — general-purpose vs. "special" (segment +
  control) register state of the vCPU.
- **Guest-physical address** — an address in the guest's physical memory; maps
  to `vm.mem + address` on the host.
- **Memory slot** — one host↦guest memory mapping registered with
  `KVM_SET_USER_MEMORY_REGION`.
- **Port I/O (PIO)** — `in`/`out` instructions; trap as `KVM_EXIT_IO`.
- **MMIO** — device access via memory reads/writes to unbacked physical
  addresses; traps as `KVM_EXIT_MMIO`.
- **16550 UART / COM1** — the legacy serial device (base port `0x3F8`) the guest
  writes text to; nutvisor turns those writes into host stdout.
- **Real / protected / long mode** — the 16/32/64-bit x86 CPU modes; which one a
  guest runs in is set by its `sregs` (segments + control registers).
- **GDT** — Global Descriptor Table; defines segments. A 64-bit code descriptor
  is needed for long mode.
- **EFER.LME / LMA** — the model-specific register bits that enable/indicate
  long mode.
- **ELF64 / PT_LOAD** — the executable format of a kernel image and the program-
  header type that says "load this segment into memory."
- **Multiboot2** — the boot protocol Nutshell uses; booting it in nutvisor means
  emulating the GRUB hand-off (a stretch goal).
- **Triple fault** — a fault while handling a fault while handling a fault; the
  CPU resets, surfacing as `KVM_EXIT_SHUTDOWN`.
