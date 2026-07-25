#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef uint32_t (*ioport_in_fn)(uint16_t port, uint8_t size);
typedef void (*ioport_out_fn)(uint16_t port, uint8_t size, uint32_t value);

struct ioport_device {
    uint16_t base;
    uint16_t size;
    uint8_t access_size;
    ioport_in_fn in;
    ioport_out_fn out;
};

bool ioport_read(uint16_t port, uint8_t size, uint32_t *value);
bool ioport_write(uint16_t port, uint8_t size, uint32_t value);
