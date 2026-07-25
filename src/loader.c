/* Strict ELF64 loader for identity-mapped x86-64 kernel guests. Validation is
 * completed before guest RAM is modified, so malformed files fail cleanly. */

#include <elf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "loader.h"
#include "vmm.h"

#define MAX_PROGRAM_HEADERS 1024U

static bool file_range_valid(uint64_t offset, uint64_t length, size_t size) {
    return offset <= size && length <= size - offset;
}

static bool guest_range_valid(uint64_t address, uint64_t length, size_t size) {
    return address <= size && length <= size - address;
}

static void read_program_header(const uint8_t *image, const Elf64_Ehdr *header,
                                size_t index, Elf64_Phdr *program) {
    uint64_t offset = header->e_phoff + index * sizeof(*program);

    memcpy(program, image + offset, sizeof(*program));
}

static int validate_header(const uint8_t *image, size_t image_size,
                           Elf64_Ehdr *header) {
    if (image_size < sizeof(*header)) {
        fprintf(stderr, "ELF: file is smaller than the ELF64 header\n");
        return -1;
    }
    memcpy(header, image, sizeof(*header));

    if (memcmp(header->e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "ELF: invalid magic\n");
        return -1;
    }
    if (header->e_ident[EI_CLASS] != ELFCLASS64) {
        fprintf(stderr, "ELF: expected a 64-bit image\n");
        return -1;
    }
    if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
        fprintf(stderr, "ELF: expected little-endian encoding\n");
        return -1;
    }
    if (header->e_ident[EI_VERSION] != EV_CURRENT
            || header->e_version != EV_CURRENT) {
        fprintf(stderr, "ELF: unsupported format version\n");
        return -1;
    }
    if (header->e_machine != EM_X86_64) {
        fprintf(stderr, "ELF: expected an x86-64 image\n");
        return -1;
    }
    if (header->e_type != ET_EXEC) {
        fprintf(stderr, "ELF: expected an executable image\n");
        return -1;
    }
    if (header->e_ehsize != sizeof(*header)
            || header->e_phentsize != sizeof(Elf64_Phdr)) {
        fprintf(stderr, "ELF: unsupported header layout\n");
        return -1;
    }
    if (header->e_phnum == 0 || header->e_phnum > MAX_PROGRAM_HEADERS) {
        fprintf(stderr, "ELF: invalid program-header count\n");
        return -1;
    }
    if (!file_range_valid(header->e_phoff,
                          (uint64_t)header->e_phnum * sizeof(Elf64_Phdr),
                          image_size)) {
        fprintf(stderr, "ELF: program-header table is outside the file\n");
        return -1;
    }
    return 0;
}

static int validate_segments(struct vm *vm, const uint8_t *image,
                             size_t image_size, const Elf64_Ehdr *header) {
    bool found_load = false;
    bool entry_loaded = false;

    for (size_t i = 0; i < header->e_phnum; i++) {
        Elf64_Phdr program;
        read_program_header(image, header, i, &program);

        if (program.p_type != PT_LOAD)
            continue;
        found_load = true;
        if (program.p_memsz < program.p_filesz) {
            fprintf(stderr, "ELF: segment %zu has filesz larger than memsz\n",
                    i);
            return -1;
        }
        if (!file_range_valid(program.p_offset, program.p_filesz,
                              image_size)) {
            fprintf(stderr, "ELF: segment %zu data is outside the file\n", i);
            return -1;
        }
        if (!guest_range_valid(program.p_paddr, program.p_memsz,
                               vm->mem_size)) {
            fprintf(stderr, "ELF: segment %zu does not fit in guest RAM\n", i);
            return -1;
        }
        if ((program.p_flags & PF_X) != 0
                && header->e_entry >= program.p_paddr
                && header->e_entry - program.p_paddr < program.p_memsz) {
            entry_loaded = true;
        }
    }

    if (!found_load) {
        fprintf(stderr, "ELF: image has no loadable segments\n");
        return -1;
    }
    if (!entry_loaded) {
        fprintf(stderr, "ELF: entry point is not in an executable segment\n");
        return -1;
    }
    return 0;
}

int elf64_load(struct vm *vm, const void *image_data, size_t image_size,
               uint64_t *entry) {
    const uint8_t *image = image_data;
    Elf64_Ehdr header;

    if (validate_header(image, image_size, &header) < 0
            || validate_segments(vm, image, image_size, &header) < 0)
        return -1;

    for (size_t i = 0; i < header.e_phnum; i++) {
        Elf64_Phdr program;
        read_program_header(image, &header, i, &program);

        if (program.p_type != PT_LOAD)
            continue;
        memcpy(vm->mem + program.p_paddr, image + program.p_offset,
               (size_t)program.p_filesz);
        memset(vm->mem + program.p_paddr + program.p_filesz, 0,
               (size_t)(program.p_memsz - program.p_filesz));
    }

    *entry = header.e_entry;
    return 0;
}
