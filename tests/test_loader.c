/* Host-only loader regression tests. Never opens /dev/kvm. */
#include <assert.h>
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "loader.h"
#include "memory_layout.h"
#include "vmm.h"

#define IMAGE_SIZE 1024U
#define RAM_SIZE (16U << 20)
#define LOAD_ADDR 0x100000U

static unsigned char image[IMAGE_SIZE];
static unsigned char *ram;
static unsigned int cases;

static Elf64_Ehdr header(void) {
    Elf64_Ehdr h;
    memcpy(&h, image, sizeof(h));
    return h;
}

static void set_header(Elf64_Ehdr h) {
    memcpy(image, &h, sizeof(h));
}

static Elf64_Phdr segment(unsigned int index) {
    Elf64_Phdr p;
    memcpy(&p, image + sizeof(Elf64_Ehdr) + index * sizeof(p), sizeof(p));
    return p;
}

static void set_segment(unsigned int index, Elf64_Phdr p) {
    memcpy(image + sizeof(Elf64_Ehdr) + index * sizeof(p), &p, sizeof(p));
}

static void fixture(void) {
    memset(image, 0, sizeof(image));
    Elf64_Ehdr h = {
        .e_type = ET_EXEC, .e_machine = EM_X86_64, .e_version = EV_CURRENT,
        .e_entry = LOAD_ADDR, .e_phoff = sizeof(h), .e_ehsize = sizeof(h),
        .e_phentsize = sizeof(Elf64_Phdr), .e_phnum = 1,
    };
    memcpy(h.e_ident, ELFMAG, SELFMAG);
    h.e_ident[EI_CLASS] = ELFCLASS64;
    h.e_ident[EI_DATA] = ELFDATA2LSB;
    h.e_ident[EI_VERSION] = EV_CURRENT;
    set_header(h);
    set_segment(0, (Elf64_Phdr) {
        .p_type = PT_LOAD, .p_flags = PF_R | PF_X,
        .p_offset = 512, .p_vaddr = LOAD_ADDR, .p_paddr = LOAD_ADDR,
        .p_filesz = 16, .p_memsz = 32, .p_align = 1,
    });
    memset(image + 512, 0xCC, 16);
}

static void reject(size_t size) {
    struct vm vm = { .mem = ram, .mem_size = RAM_SIZE };
    uint64_t entry = UINT64_MAX;
    memset(ram, 0xA5, RAM_SIZE);
    assert(elf64_validate(image, size, RAM_SIZE, &entry) == -1);
    assert(entry == UINT64_MAX);
    assert(elf64_load(&vm, image, size, &entry) == -1);
    assert(entry == UINT64_MAX);
    /* Check the entire mapping: a late failure must not leave partial loads. */
    for (size_t i = 0; i < RAM_SIZE; i++)
        assert(ram[i] == 0xA5);
    cases++;
}

static void accept(void) {
    struct vm vm = { .mem = ram, .mem_size = RAM_SIZE };
    uint64_t entry = 0;
    memset(ram, 0xA5, RAM_SIZE);
    assert(elf64_load(&vm, image, sizeof(image), &entry) == 0);
    assert(entry == header().e_entry);
    cases++;
}

static void test_headers(void) {
    fixture(); reject(0);
    fixture(); reject(sizeof(Elf64_Ehdr) - 1);
    fixture(); image[0] = 0; reject(sizeof(image));
    fixture(); image[EI_CLASS] = ELFCLASS32; reject(sizeof(image));
    fixture(); image[EI_DATA] = ELFDATA2MSB; reject(sizeof(image));
    fixture(); image[EI_VERSION] = 0; reject(sizeof(image));
#define BAD_HEADER(field, value) do { \
    fixture(); Elf64_Ehdr h = header(); h.field = (value); \
    set_header(h); reject(sizeof(image)); \
} while (0)
    BAD_HEADER(e_machine, EM_AARCH64);
    BAD_HEADER(e_type, ET_DYN);
    BAD_HEADER(e_version, 0);
    BAD_HEADER(e_ehsize, 0);
    BAD_HEADER(e_phentsize, 0);
    BAD_HEADER(e_phnum, 0);
    BAD_HEADER(e_phnum, 1025);
    BAD_HEADER(e_phoff, UINT64_MAX);
    BAD_HEADER(e_phoff, IMAGE_SIZE - sizeof(Elf64_Phdr) + 1);
    BAD_HEADER(e_entry, LOAD_ADDR + 16); /* BSS is not entry code. */
    BAD_HEADER(e_entry, UINT64_MAX);
#undef BAD_HEADER
}

static void test_segments(void) {
#define BAD_SEGMENT(field, value) do { \
    fixture(); Elf64_Phdr p = segment(0); p.field = (value); \
    set_segment(0, p); reject(sizeof(image)); \
} while (0)
    BAD_SEGMENT(p_type, PT_NOTE);
    BAD_SEGMENT(p_type, PT_INTERP);
    BAD_SEGMENT(p_type, PT_DYNAMIC);
    BAD_SEGMENT(p_flags, PF_R);
    BAD_SEGMENT(p_memsz, 15);
    BAD_SEGMENT(p_offset, UINT64_MAX);
    BAD_SEGMENT(p_offset, IMAGE_SIZE - 15);
    BAD_SEGMENT(p_memsz, UINT64_MAX);
    BAD_SEGMENT(p_vaddr, LOAD_ADDR + 1);
    BAD_SEGMENT(p_align, 3);
    BAD_SEGMENT(p_align, 4096); /* offset is not congruent with vaddr. */
#undef BAD_SEGMENT
    const uint64_t bad_addresses[] = {
        UINT64_MAX, RAM_SIZE, VM_PAGE_TABLE_PML4 - 1,
        VM_PAGE_TABLE_PML4, VM_BOOT_TABLES_END - 1,
        VM_LONG_MODE_STACK - VM_STACK_SIZE - 1,
        VM_LONG_MODE_STACK - 1,
    };
    for (size_t i = 0; i < sizeof(bad_addresses) / sizeof(bad_addresses[0]); i++) {
        fixture();
        Elf64_Phdr p = segment(0);
        p.p_vaddr = p.p_paddr = bad_addresses[i];
        set_segment(0, p);
        Elf64_Ehdr h = header(); h.e_entry = bad_addresses[i]; set_header(h);
        reject(sizeof(image));
    }
    /* Adjacent ranges, exact file end, legal zero/one alignment and a
     * non-aligned but congruent address/offset must remain accepted. */
    const uint64_t good_addresses[] = {
        VM_PAGE_TABLE_PML4 - 32, VM_BOOT_TABLES_END,
        VM_LONG_MODE_STACK - VM_STACK_SIZE - 32,
        VM_LONG_MODE_STACK, RAM_SIZE - 32,
    };
    for (size_t i = 0; i < sizeof(good_addresses) / sizeof(good_addresses[0]); i++) {
        fixture();
        Elf64_Phdr p = segment(0);
        p.p_vaddr = p.p_paddr = good_addresses[i];
        p.p_align = 0;
        p.p_offset = IMAGE_SIZE - p.p_filesz;
        set_segment(0, p);
        Elf64_Ehdr h = header(); h.e_entry = p.p_paddr; set_header(h);
        accept();
    }
    fixture();
    Elf64_Phdr p = segment(0);
    p.p_vaddr = p.p_paddr = LOAD_ADDR + 512; p.p_align = 4096;
    set_segment(0, p);
    Elf64_Ehdr h = header(); h.e_entry = p.p_paddr; set_header(h);
    accept();
}

static void test_multiple_segments(void) {
    fixture();
    Elf64_Ehdr h = header(); h.e_phnum = 2; set_header(h);
    Elf64_Phdr p = segment(0);
    p.p_paddr = p.p_vaddr = LOAD_ADDR + 31; /* First segment's BSS tail. */
    set_segment(1, p);
    reject(sizeof(image));
    p.p_paddr = p.p_vaddr = LOAD_ADDR - 32; /* Disjoint but out of order. */
    set_segment(1, p);
    reject(sizeof(image));
    p.p_paddr = p.p_vaddr = LOAD_ADDR + 32;
    set_segment(1, p);
    accept();
    p.p_memsz = p.p_filesz = 0;
    p.p_paddr = p.p_vaddr = VM_PAGE_TABLE_PML4; /* Empty: no reservation hit. */
    set_segment(1, p);
    accept();
    p.p_type = PT_NOTE;
    set_segment(1, p);
    accept();
    p.p_type = PT_INTERP;
    set_segment(1, p);
    reject(sizeof(image));
}

int main(void) {
    ram = malloc(RAM_SIZE);
    assert(ram);
    fixture(); accept();
    assert(ram[LOAD_ADDR - 1] == 0xA5);
    for (unsigned int i = 0; i < 16; i++) assert(ram[LOAD_ADDR + i] == 0xCC);
    for (unsigned int i = 16; i < 32; i++) assert(ram[LOAD_ADDR + i] == 0);
    assert(ram[LOAD_ADDR + 32] == 0xA5);
    test_headers();
    test_segments();
    test_multiple_segments();
    /* Headers may start at an unaligned host address. */
    fixture();
    unsigned char unaligned[IMAGE_SIZE + 1];
    memcpy(unaligned + 1, image, IMAGE_SIZE);
    uint64_t entry = 0;
    assert(elf64_validate(unaligned + 1, IMAGE_SIZE, RAM_SIZE, &entry) == 0);
    assert(elf64_validate(NULL, IMAGE_SIZE, RAM_SIZE, &entry) == -1);
    assert(elf64_validate(image, IMAGE_SIZE, RAM_SIZE, NULL) == -1);
    struct vm invalid_vm = { .mem = MAP_FAILED, .mem_size = RAM_SIZE };
    assert(elf64_load(&invalid_vm, image, IMAGE_SIZE, &entry) == -1);
    assert(elf64_load(NULL, image, IMAGE_SIZE, &entry) == -1);
    free(ram);
    printf("[ok] loader: %u fixtures, atomic rejection, BSS and unaligned headers\n", cases);
    return 0;
}
