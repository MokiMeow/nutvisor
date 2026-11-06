/* Host-only tests of the real devices, with no KVM or console output paths.
 * gcc -std=gnu11 -Wall -Wextra -Werror -Iinclude tests/test_devices.c
 *     src/ioport.c src/serial.c src/mmio.c -o /tmp/test_devices */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ioport.h"
#include "mmio.h"
#include "serial.h"

static unsigned checks;
#define CHECK(condition) do { \
    checks++; \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s failed\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static void uart_write(unsigned offset, uint32_t value) {
    CHECK(ioport_write(SERIAL_COM1_BASE + offset, 1, value));
}

static uint32_t uart_read(unsigned offset) {
    uint32_t value = UINT32_MAX;
    CHECK(ioport_read(SERIAL_COM1_BASE + offset, 1, &value));
    return value;
}

static void check_reset(void) {
    static const uint32_t expected[] = { 0, 0, 1, 0, 0, 0x60, 0, 0 };
    serial_reset();
    for (unsigned i = 0; i < SERIAL_COM1_SIZE; i++)
        CHECK(uart_read(i) == expected[i]);
    uart_write(3, 0x80);
    CHECK(uart_read(0) == 0 && uart_read(1) == 0);
    uart_write(3, 0);
}

static void test_uart_registers(void) {
    check_reset();
    uart_write(1, 0x0b);             /* IER without DLAB. */
    uart_write(3, 0x83);
    uart_write(0, 0x12345678);       /* Divisor writes do not print. */
    uart_write(1, 0xabcdef12);
    CHECK(uart_read(0) == 0x78);
    CHECK(uart_read(1) == 0x12);
    CHECK(uart_read(3) == 0x83);
    uart_write(3, 3);
    CHECK(uart_read(0) == 0);        /* No received data. */
    CHECK(uart_read(1) == 0x0b);     /* Divisor high did not overwrite IER. */
    uart_write(1, 0x04);
    uart_write(3, 0x83);
    CHECK(uart_read(1) == 0x12);     /* IER did not overwrite divisor high. */
    CHECK(uart_read(0) == 0x78);
    uart_write(3, 3);
    CHECK(uart_read(1) == 0x04);
    uart_write(7, 0xabcdefa5);
    CHECK(uart_read(7) == 0xa5);
    uart_write(4, 0x1234561f);
    CHECK(uart_read(4) == 0x1f);
    uart_write(2, 0xff);             /* FCR writes do not change IIR. */
    CHECK(uart_read(2) == 1);
    uart_write(5, 0xff);             /* Read-only LSR/MSR ignore writes. */
    uart_write(6, 0xff);
    CHECK(uart_read(5) == 0x60 && uart_read(6) == 0);
    check_reset();                  /* Includes both divisor bytes. */
}

static void test_uart_invalid_access(void) {
    static const uint16_t ports[] = {
        0, SERIAL_COM1_BASE - 1, SERIAL_COM1_BASE + SERIAL_COM1_SIZE,
        UINT16_MAX
    };
    serial_reset();
    uart_write(7, 0x5a);
    for (size_t i = 0; i < sizeof(ports) / sizeof(ports[0]); i++) {
        uint32_t value = 0xdeadbeef;
        CHECK(!ioport_read(ports[i], 1, &value));
        CHECK(value == 0xdeadbeef);
        CHECK(!ioport_write(ports[i], 1, 0xff));
        CHECK(serial_in(ports[i], 1) == 0);
        serial_out(ports[i], 1, 0xff);
        CHECK(uart_read(7) == 0x5a);
    }
    for (unsigned size = 0; size <= UINT8_MAX; size++) {
        if (size == 1)
            continue;
        uint32_t value = 0xdeadbeef;
        CHECK(!ioport_read(SERIAL_COM1_BASE + 7, size, &value));
        CHECK(value == 0xdeadbeef);
        CHECK(!ioport_write(SERIAL_COM1_BASE + 7, size, 0xff));
        CHECK(serial_in(SERIAL_COM1_BASE + 7, size) == UINT32_MAX);
        serial_out(SERIAL_COM1_BASE + 7, size, 0xff);
        CHECK(uart_read(7) == 0x5a);
    }
}

static void reject_mmio(uint64_t address, bool write, size_t len) {
    uint8_t data[10];
    uint8_t expected[sizeof(data)];
    uint32_t status = 0xdeadbeef;
    memset(data, 0xa5, sizeof(data));
    memcpy(expected, data, sizeof(data));
    CHECK(mmio_access(address, write, data + 1, len, &status) == MMIO_UNHANDLED);
    CHECK(memcmp(data, expected, sizeof(data)) == 0);
    CHECK(status == 0xdeadbeef);
}

static void test_mmio_read(void) {
    /* Every in-range offset and valid length, including exact end bounds. */
    for (unsigned offset = 0; offset < 16; offset++) {
        for (size_t len = 1; len <= 8 && len <= 16 - offset; len++) {
            uint8_t data[10];
            uint32_t status = 0xdeadbeef;
            memset(data, 0xa5, sizeof(data));
            CHECK(mmio_access(MMIO_DEVICE_BASE + offset, false,
                              data + 1, len, &status) == MMIO_CONTINUE);
            CHECK(status == 0xdeadbeef);
            CHECK(data[0] == 0xa5);
            for (size_t i = 1; i <= len; i++)
                CHECK(data[i] == 0);
            for (size_t i = len + 1; i < sizeof(data); i++)
                CHECK(data[i] == 0xa5);
        }
    }
}

static void test_mmio_exit(void) {
    static const size_t lengths[] = { 1, 2, 4 };
    static const uint32_t expected[] = { 0x78, 0x5678, 0x12345678 };
    for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
        uint8_t data[] = { 0xa5, 0x78, 0x56, 0x34, 0x12, 0xa5 };
        uint8_t original[sizeof(data)];
        uint32_t status = UINT32_MAX;
        memcpy(original, data, sizeof(data));
        CHECK(mmio_access(MMIO_EXIT_ADDR, true, data + 1, lengths[i],
                          &status) == MMIO_STOP);
        CHECK(status == expected[i]);
        CHECK(memcmp(data, original, sizeof(data)) == 0);
        memset(data + 1, 0, 4);
        status = UINT32_MAX;
        CHECK(mmio_access(MMIO_EXIT_ADDR, true, data + 1, lengths[i],
                          &status) == MMIO_STOP);
        CHECK(status == 0);
    }
}

static void test_mmio_invalid_access(void) {
    static const uint64_t addresses[] = {
        0, MMIO_DEVICE_BASE - 1, MMIO_DEVICE_BASE + 16,
        UINT64_MAX - 3, UINT64_MAX
    };
    static const size_t invalid_lengths[] = { 0, 9, SIZE_MAX };
    for (unsigned write = 0; write <= 1; write++) {
        for (size_t i = 0; i < sizeof(addresses) / sizeof(addresses[0]); i++)
            reject_mmio(addresses[i], write, 1);
        for (size_t i = 0; i < sizeof(invalid_lengths) / sizeof(invalid_lengths[0]); i++)
            reject_mmio(MMIO_EXIT_ADDR, write, invalid_lengths[i]);
        for (unsigned offset = 9; offset < 16; offset++)
            reject_mmio(MMIO_DEVICE_BASE + offset, write, 17 - offset);
    }
    for (unsigned offset = 1; offset < 16; offset++) {
        if (offset != 8)
            reject_mmio(MMIO_DEVICE_BASE + offset, true, 1);
    }
    for (size_t len = 3; len <= 8; len++) {
        if (len != 4)
            reject_mmio(MMIO_EXIT_ADDR, true, len);
    }
}

int main(void) {
    test_uart_registers();
    test_uart_invalid_access();
    test_mmio_read();
    test_mmio_exit();
    test_mmio_invalid_access();
    printf("Device tests passed: 5 groups, %u checks\n", checks);
    return 0;
}
