# 07 — Device emulation

*Serial is milestone 0/1; MMIO is milestone 3.*

A guest "talks to hardware" by executing I/O instructions or touching special
memory. Each of those traps out as a VM exit, and the host code that answers it
*is* the device. There is no magic — a device is a function behind an exit.

## Port-I/O devices (serial, milestone 0/1)

The guest's `out dx, al` / `in al, dx` become `KVM_EXIT_IO`. The exit says which
`port`, the `direction`, the access `size`, the `count`, and where the bytes are
(`data_offset` into the `run` mapping). nutvisor's COM1 model turns writes to
`0x3F8` into `putchar`:

```c
if (serial_handles_port(port)) {
    for (i = 0; i < count; i++)
        if (direction == KVM_EXIT_IO_OUT) serial_out(port, data[i*size]);
        else                              data[i*size] = serial_in(port);
}
```

A fuller 16550 (milestone 1) also answers reads of the Line Status Register
(`0x3FD`) with "ready" so a guest's *driver* — which polls LSR before writing —
works, and accepts the config writes (LCR/IER/FIFO) as no-ops. A real kernel
guest uses a driver, not raw `out`s, so this matters once the guest is Nutshell.

### The dispatch table (milestone 1)

Instead of an `if` per device, register `{port_range, in_fn, out_fn}` entries
and let `vm.c` route each I/O exit to the owning device. This is how you add a
second port device cleanly.

## Memory-mapped devices (MMIO, milestone 3)

Some devices live at a physical *address* instead of a port. If the guest reads
or writes physical memory that isn't backed by a memory slot, KVM returns
`KVM_EXIT_MMIO` with `phys_addr`, `len`, `is_write`, and a `data[8]` buffer:

```c
case KVM_EXIT_MMIO:
    if (mmio_handles(run->mmio.phys_addr)) {
        if (run->mmio.is_write) mmio_write(addr, run->mmio.data, len);
        else                    mmio_read(addr, run->mmio.data, len);
    }
```

You choose an address range that is *deliberately not* mapped as RAM, so
accesses trap. A good first MMIO device is a debug console (write a byte → host
prints it) or an "exit" device (write a code → stop the VM), which is handy for
self-tests.

## Principles

- One device per file; each declares the ports/addresses it owns.
- Devices never call KVM; they only read/write the buffers the exit hands them.
- Keep reads honest — returning a plausible status register is often what makes
  a guest *driver* progress instead of hanging.
