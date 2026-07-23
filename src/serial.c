/* COM1 device model. The guest talks to a serial port; the host turns those
 * port writes into characters on stdout. This is the VMM side of the same
 * 16550 UART that a real kernel driver would program. */

#include <stdio.h>

#include "serial.h"

#define COM1_BASE 0x3F8
#define COM1_DATA (COM1_BASE + 0) /* transmit/receive */
#define COM1_LSR  (COM1_BASE + 5) /* line status register */

int serial_handles_port(uint16_t port) {
    return port >= COM1_BASE && port < COM1_BASE + 8;
}

void serial_out(uint16_t port, uint8_t value) {
    if (port == COM1_DATA) {
        putchar((int)value);
        fflush(stdout);
    }
    /* Other registers (LCR, IER, FIFO control) are accepted and ignored at
     * milestone 0; the guest's configuration writes are harmless no-ops. */
}

uint8_t serial_in(uint16_t port) {
    if (port == COM1_LSR)
        return 0x60; /* THR empty + transmitter idle: always ready to send */
    return 0;
}
