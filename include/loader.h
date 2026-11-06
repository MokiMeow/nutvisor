#pragma once

#include <stddef.h>
#include <stdint.h>

struct vm;

/* Validate without KVM or guest RAM. Output is unchanged on failure. */
int elf64_validate(const void *image, size_t image_size, size_t mem_size,
                    uint64_t *entry);

/* Image storage must not overlap guest RAM. Failed validation changes neither
 * guest RAM nor entry. Only static, identity-mapped ELF64 guests are supported. */
int elf64_load(struct vm *vm, const void *image, size_t image_size,
               uint64_t *entry);
