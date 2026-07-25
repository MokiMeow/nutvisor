/* Memory-mapped debug/exit device. Its page is deliberately absent from the
 * KVM RAM slot so accesses return to userspace as KVM_EXIT_MMIO. */

#include <stdio.h>
#include <string.h>

#include "mmio.h"

#define MMIO_DEVICE_SIZE 16U

typedef enum mmio_result (*mmio_read_fn)(uint64_t offset, uint8_t *data,
                                         size_t len);
typedef enum mmio_result (*mmio_write_fn)(uint64_t offset, const uint8_t *data,
                                          size_t len, uint32_t *exit_status);

struct mmio_device {
    uint64_t base;
    uint64_t size;
    mmio_read_fn read;
    mmio_write_fn write;
};

static enum mmio_result control_read(uint64_t offset, uint8_t *data,
                                     size_t len) {
    (void)offset;
    memset(data, 0, len);
    return MMIO_CONTINUE;
}

static enum mmio_result control_write(uint64_t offset, const uint8_t *data,
                                      size_t len, uint32_t *exit_status) {
    if (offset == MMIO_DEBUG_ADDR - MMIO_DEVICE_BASE) {
        for (size_t i = 0; i < len; i++)
            putchar((int)data[i]);
        fflush(stdout);
        return MMIO_CONTINUE;
    }

    if (offset == MMIO_EXIT_ADDR - MMIO_DEVICE_BASE
            && (len == 1 || len == 2 || len == 4)) {
        uint32_t status = 0;

        memcpy(&status, data, len);
        *exit_status = status;
        return MMIO_STOP;
    }
    return MMIO_UNHANDLED;
}

static const struct mmio_device devices[] = {
    { MMIO_DEVICE_BASE, MMIO_DEVICE_SIZE, control_read, control_write },
};

enum mmio_result mmio_access(uint64_t address, bool is_write, uint8_t *data,
                             size_t len, uint32_t *exit_status) {
    if (len == 0 || len > 8)
        return MMIO_UNHANDLED;

    for (size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
        const struct mmio_device *device = &devices[i];

        if (address < device->base
                || address - device->base >= device->size
                || len > device->size - (address - device->base))
            continue;
        if (is_write)
            return device->write(address - device->base, data, len,
                                 exit_status);
        return device->read(address - device->base, data, len);
    }
    return MMIO_UNHANDLED;
}
