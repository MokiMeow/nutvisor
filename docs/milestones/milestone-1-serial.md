# Milestone 1 — Serial device model ✅ (done)

**Goal:** a proper port-I/O dispatch layer and a fuller 16550 COM1 model, so a
guest *driver* (not just raw `out`s) works — the foundation for a kernel guest.

## Concepts

Port-I/O VM exits in depth, the 16550 register set (THR/RBR, LSR, LCR, IER, FCR),
and a device-dispatch table so `vm.c` doesn't grow an `if` per device.

## Tasks

- [x] Add a port-I/O dispatch table: entries of `{ base, size, in_fn, out_fn }`;
      `vm.c`'s `KVM_EXIT_IO` case looks up the owning device and calls it.
- [x] Expand `serial.c` to a fuller COM1: honour the data register for output,
      return the Line Status Register (`0x3FD`) as "transmit ready", and accept
      LCR/IER/FIFO-control writes as no-ops (so a real driver's setup succeeds).
- **Deferred optional (post-v1):** feed host stdin to the guest via the receive
  register + LSR data-ready bit, so an interactive guest can read input.
- [x] Add a guest (`guests/serial-driver.asm` or C) that programs the UART like
      a driver would (poll LSR, then write) and prints a message — proving the
      model, not just raw `out`.

## Files

`src/vm.c` (dispatch table), `src/serial.c` + `include/serial.h` (fuller model),
`include/ioport.h` (dispatch types), `guests/serial-driver.*`.

## Definition of Done

- [x] `make run` with the driver-style guest prints its message correctly.
- [x] Adding a second port device requires only a new table entry, no `vm.c`
      surgery.
- [x] `make all` clean; milestone-0 guest still runs.

## References

- OSDev Wiki — [Serial Ports](https://wiki.osdev.org/Serial_Ports)
- [docs/07 — Device emulation](../07-device-emulation.md)

**Next:** [Milestone 2 — Long mode](milestone-2-longmode.md).
