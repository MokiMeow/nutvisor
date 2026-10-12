/* Avoid offset + count * size overflow for KVM shared-memory I/O exits. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "io_bounds.h"

bool vm_io_span_valid(size_t mapping_size, uint64_t data_offset,
                      uint32_t count, uint8_t access_size) {
    if (access_size == 0 || access_size > sizeof(uint32_t) || count == 0)
        return false;
    if (data_offset > mapping_size)
        return false;
    /* Divide rather than multiply or add potentially overflowing values. */
    return count <= (mapping_size - (size_t)data_offset) / access_size;
}
