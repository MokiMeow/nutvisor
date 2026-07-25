#pragma once
#include <stdint.h>

#define SERIAL_COM1_BASE 0x3F8U
#define SERIAL_COM1_SIZE 8U

void serial_reset(void);
void serial_out(uint16_t port, uint8_t size, uint32_t value);
uint32_t serial_in(uint16_t port, uint8_t size);
