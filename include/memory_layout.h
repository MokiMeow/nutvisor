#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Shared bootstrap contract: keep loader validation and vCPU setup in sync. */
#define VM_PAGE_SIZE       0x1000ULL
#define VM_PAGE_TABLE_PML4 0x2000ULL
#define VM_PAGE_TABLE_PDPT 0x3000ULL
#define VM_PAGE_TABLE_PD   0x4000ULL
#define VM_GDT_ADDRESS     0x5000ULL
#define VM_BOOT_TABLES_END 0x6000ULL
#define VM_LONG_MODE_STACK 0x800000ULL
#define VM_STACK_SIZE      0x10000ULL

/* Subtraction avoids wrapping address + length for hostile image metadata. */
static inline bool vm_range_fits(uint64_t address, uint64_t length,
                                  uint64_t limit) {
    return address <= limit && length <= limit - address;
}

static inline bool vm_range_overlaps(uint64_t address, uint64_t length,
                                      uint64_t start, uint64_t end) {
    return length != 0 && address < end
        && (address >= start || length > start - address);
}

static inline bool vm_boot_range_reserved(uint64_t address, uint64_t length) {
    return vm_range_overlaps(address, length, VM_PAGE_TABLE_PML4,
                             VM_BOOT_TABLES_END)
        || vm_range_overlaps(address, length,
                              VM_LONG_MODE_STACK - VM_STACK_SIZE,
                              VM_LONG_MODE_STACK);
}
