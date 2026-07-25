#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MMIO_DEVICE_BASE 0x10000000ULL
#define MMIO_DEBUG_ADDR  (MMIO_DEVICE_BASE + 0U)
#define MMIO_EXIT_ADDR   (MMIO_DEVICE_BASE + 8U)

enum mmio_result {
    MMIO_UNHANDLED = -1,
    MMIO_CONTINUE = 0,
    MMIO_STOP = 1,
};

enum mmio_result mmio_access(uint64_t address, bool is_write, uint8_t *data,
                             size_t len, uint32_t *exit_status);
