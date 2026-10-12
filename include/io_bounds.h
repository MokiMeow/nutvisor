#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Validate the KVM_EXIT_IO byte span before accessing a shared kvm_run map. */
bool vm_io_span_valid(size_t mapping_size, uint64_t data_offset,
                      uint32_t count, uint8_t access_size);
