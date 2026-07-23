#pragma once
#include <stdint.h>

/* Minimal 16550-style COM1 device (ports 0x3F8-0x3FF).
 * Milestone 0 only needs byte output; milestone 1 expands this into a fuller
 * register model (LCR/IER/FIFO, and optional input). */

int     serial_handles_port(uint16_t port);
void    serial_out(uint16_t port, uint8_t value);
uint8_t serial_in(uint16_t port);
