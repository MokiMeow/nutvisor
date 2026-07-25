/* 16550-compatible COM1 model. It implements the register behaviour needed
 * by a polling guest driver while forwarding transmitted bytes to stdout. */

#include <stdio.h>
#include <string.h>

#include "serial.h"

#define UART_RBR_THR_DLL 0U
#define UART_IER_DLM     1U
#define UART_IIR_FCR     2U
#define UART_LCR         3U
#define UART_MCR         4U
#define UART_LSR         5U
#define UART_MSR         6U
#define UART_SCR         7U

#define UART_LCR_DLAB 0x80U
#define UART_LSR_THRE 0x20U
#define UART_LSR_TEMT 0x40U

struct serial_state {
    uint8_t ier;
    uint8_t fcr;
    uint8_t lcr;
    uint8_t mcr;
    uint8_t scratch;
    uint8_t divisor_low;
    uint8_t divisor_high;
};

static struct serial_state state;

void serial_reset(void) {
    memset(&state, 0, sizeof(state));
}

void serial_out(uint16_t port, uint8_t size, uint32_t value) {
    uint16_t offset = port - SERIAL_COM1_BASE;
    uint8_t byte = (uint8_t)value;

    if (size != 1)
        return;

    switch (offset) {
    case UART_RBR_THR_DLL:
        if (state.lcr & UART_LCR_DLAB) {
            state.divisor_low = byte;
            return;
        }
        putchar((int)byte);
        fflush(stdout);
        return;
    case UART_IER_DLM:
        if (state.lcr & UART_LCR_DLAB)
            state.divisor_high = byte;
        else
            state.ier = byte;
        return;
    case UART_IIR_FCR:
        state.fcr = byte;
        return;
    case UART_LCR:
        state.lcr = byte;
        return;
    case UART_MCR:
        state.mcr = byte;
        return;
    case UART_SCR:
        state.scratch = byte;
        return;
    default:
        return;
    }
}

uint32_t serial_in(uint16_t port, uint8_t size) {
    uint16_t offset = port - SERIAL_COM1_BASE;

    if (size != 1)
        return UINT32_MAX;

    switch (offset) {
    case UART_RBR_THR_DLL:
        return (state.lcr & UART_LCR_DLAB) ? state.divisor_low : 0U;
    case UART_IER_DLM:
        return (state.lcr & UART_LCR_DLAB) ? state.divisor_high : state.ier;
    case UART_IIR_FCR:
        return 0x01U; /* no interrupt pending */
    case UART_LCR:
        return state.lcr;
    case UART_MCR:
        return state.mcr;
    case UART_LSR:
        return UART_LSR_THRE | UART_LSR_TEMT;
    case UART_MSR:
        return 0U;
    case UART_SCR:
        return state.scratch;
    default:
        return 0U;
    }
}
