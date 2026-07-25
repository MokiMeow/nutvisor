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

A fuller 16550 (milestone 1) answers reads of the Line Status Register
(`0x3FD`) with "ready" so a guest's *driver* — which polls LSR before writing —
works, tracks DLAB/divisor, LCR, IER, FIFO, modem-control, and scratch-register
writes, and keeps configuration bytes from leaking to host output. A real kernel
guest uses a driver, not raw `out`s, so this matters once the guest is Nutshell.

### The dispatch table (milestone 1)

`src/ioport.c` owns a table of `{base, size, access_size, in_fn, out_fn}`
entries. The vCPU loop calls the generic router, so adding another port device
only requires registering its range and callbacks; `vm.c` remains unchanged.

## Memory-mapped devices (MMIO, milestone 3)

Some devices live at a physical *address* instead of a port. If the guest reads
or writes mapped physical memory that isn't backed by a memory slot, KVM returns
`KVM_EXIT_MMIO` with `phys_addr`, `len`, `is_write`, and a `data[8]` buffer:

```c
case KVM_EXIT_MMIO:
    if (mmio_handles(run->mmio.phys_addr)) {
        if (run->mmio.is_write) mmio_write(addr, run->mmio.data, len);
        else                    mmio_read(addr, run->mmio.data, len);
    }
```

nutvisor reserves `0x10000000`–`0x1000000f`, inside the guest's identity map but
outside its 64 MiB RAM slot. Byte writes at the base form a debug console.
Writes of a 1-, 2-, or 4-byte status at base+8 stop the VM; status zero is a
clean device exit and a nonzero status fails the run. `src/mmio.c` owns the
address-range dispatch table.

## Principles

- One device per file; each declares the ports/addresses it owns.
- Devices never call KVM; they only read/write the buffers the exit hands them.
- Keep reads honest — returning a plausible status register is often what makes
  a guest *driver* progress instead of hanging.
