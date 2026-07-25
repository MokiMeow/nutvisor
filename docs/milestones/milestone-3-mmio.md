# Milestone 3 — MMIO device ✅ (done)

**Goal:** handle `KVM_EXIT_MMIO` and emulate a memory-mapped device, so guests
can talk to "hardware" by address as well as by port.

## Concepts

Memory-mapped I/O, how an unbacked guest-physical range produces an MMIO exit,
and a device that lives at an address instead of a port.

## Tasks

- [x] Choose a guest-physical range that is deliberately **not** backed by a
      memory slot (so accesses trap), e.g. `0xE000_0000`.
- [x] Handle `KVM_EXIT_MMIO` in `vm.c`: dispatch on `run->mmio.phys_addr`,
      `is_write`, `len`, and the `data` buffer, routing to an owning device.
- [x] Implement `src/mmio.c` with at least one device:
  - a **debug console**: a write of a byte to the base address prints it; and/or
  - an **exit device**: a write of a status code stops the VM (useful for
    self-tests in milestone 6).
- [x] Add a guest that writes to the MMIO device and confirm the host sees it.

## Files

`src/mmio.c` + `include/mmio.h`, `src/vm.c` (MMIO case + dispatch),
`guests/mmio-demo.*`.

## Definition of Done

- [x] A guest writing to the MMIO console prints on the host; a write to the
      exit device stops the VM with the expected status.
- [x] `make all` clean; port-I/O guests still run.

## References

- Linux KVM API — `KVM_EXIT_MMIO` in [api.rst](https://www.kernel.org/doc/html/latest/virt/kvm/api.html)
- [docs/07 — Device emulation](../07-device-emulation.md)

**Next:** [Milestone 4 — ELF loader](milestone-4-elf-loader.md).
