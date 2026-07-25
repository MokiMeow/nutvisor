/* Port-I/O routing lives outside the vCPU loop so adding a device only
 * requires registering one range here. */

#include <stddef.h>

#include "ioport.h"
#include "serial.h"

static const struct ioport_device devices[] = {
    { SERIAL_COM1_BASE, SERIAL_COM1_SIZE, 1U, serial_in, serial_out },
};

static const struct ioport_device *find_device(uint16_t port) {
    for (size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
        uint32_t end = (uint32_t)devices[i].base + devices[i].size;
        if (port >= devices[i].base && (uint32_t)port < end)
            return &devices[i];
    }
    return NULL;
}

bool ioport_read(uint16_t port, uint8_t size, uint32_t *value) {
    const struct ioport_device *device = find_device(port);

    if (!device || !device->in || size != device->access_size)
        return false;
    *value = device->in(port, size);
    return true;
}

bool ioport_write(uint16_t port, uint8_t size, uint32_t value) {
    const struct ioport_device *device = find_device(port);

    if (!device || !device->out || size != device->access_size)
        return false;
    device->out(port, size, value);
    return true;
}
