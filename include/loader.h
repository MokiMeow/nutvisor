#pragma once

#include <stddef.h>
#include <stdint.h>

struct vm;

int elf64_load(struct vm *vm, const void *image, size_t image_size,
               uint64_t *entry);
