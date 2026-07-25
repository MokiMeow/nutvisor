/* nutvisor entry point: load a guest image from a file and run it. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loader.h"
#include "vmm.h"

#define REAL_GUEST_LOAD_ADDR 0x1000ULL
#define LONG_GUEST_LOAD_ADDR 0x100000ULL
#define GUEST_MEM_SIZE       (64U << 20) /* 64 MiB of guest RAM */

enum guest_mode {
    GUEST_MODE_REAL,
    GUEST_MODE_LONG,
    GUEST_MODE_ELF,
};

static int has_suffix(const char *text, const char *suffix) {
    size_t text_len = strlen(text);
    size_t suffix_len = strlen(suffix);

    return text_len >= suffix_len
        && strcmp(text + text_len - suffix_len, suffix) == 0;
}

static long read_file(const char *path, unsigned char **out) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return -1; }

    if (fseek(f, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(f);
        return -1;
    }
    long size = ftell(f);
    if (size <= 0) {
        if (size < 0)
            perror("ftell");
        else
            fprintf(stderr, "%s: guest image is empty\n", path);
        fclose(f);
        return -1;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        perror("fseek");
        fclose(f);
        return -1;
    }

    unsigned char *buf = malloc((size_t)size);
    if (!buf) {
        perror("malloc guest image");
        fclose(f);
        return -1;
    }
    if (fread(buf, 1, (size_t)size, f) != (size_t)size) {
        if (ferror(f))
            perror("read guest image");
        else
            fprintf(stderr, "%s: unexpected end of file\n", path);
        free(buf);
        fclose(f);
        return -1;
    }
    fclose(f);
    *out = buf;
    return size;
}

int main(int argc, char **argv) {
    const char *path = "guests/kernel/kernel.elf";
    enum guest_mode mode = GUEST_MODE_ELF;

    if (argc == 2) {
        path = argv[1];
        mode = has_suffix(path, ".elf") ? GUEST_MODE_ELF : GUEST_MODE_REAL;
    } else if (argc == 3
            && (strcmp(argv[1], "--real") == 0
                || strcmp(argv[1], "--long") == 0
                || strcmp(argv[1], "--elf") == 0)) {
        if (strcmp(argv[1], "--elf") == 0)
            mode = GUEST_MODE_ELF;
        else if (strcmp(argv[1], "--long") == 0)
            mode = GUEST_MODE_LONG;
        else
            mode = GUEST_MODE_REAL;
        path = argv[2];
    } else if (argc != 1) {
        fprintf(stderr, "usage: %s [--real|--long|--elf] [guest-image]\n",
                argv[0]);
        return 2;
    }

    uint64_t load_addr = mode == GUEST_MODE_LONG
        ? LONG_GUEST_LOAD_ADDR : REAL_GUEST_LOAD_ADDR;
    unsigned char *code;
    long len = read_file(path, &code);
    if (len < 0)
        return 1;

    struct vm vm;
    int rc = VM_RUN_ERROR;
    uint64_t entry = load_addr;
    int loaded = -1;

    if (vm_create(&vm, GUEST_MEM_SIZE) == 0) {
        if (mode == GUEST_MODE_ELF)
            loaded = elf64_load(&vm, code, (size_t)len, &entry);
        else
            loaded = vm_load_guest(&vm, load_addr, code, (size_t)len);
    }

    if (loaded == 0
            && (mode == GUEST_MODE_REAL
                ? vm_set_real_mode(&vm, entry)
                : vm_set_long_mode(&vm, entry)) == 0) {
        const char *mode_name = mode == GUEST_MODE_REAL
            ? "16-bit real" : "64-bit long";

        fprintf(stderr, "nutvisor: running %s (%ld bytes) in %s mode%s\n",
                path, len, mode_name,
                mode == GUEST_MODE_ELF ? " from ELF" : "");
        rc = vm_run(&vm);
        if (rc == VM_RUN_HALTED)
            fprintf(stderr, "nutvisor: guest halted cleanly\n");
        else if (rc == VM_RUN_EXITED)
            fprintf(stderr, "nutvisor: guest exited cleanly\n");
    }

    vm_destroy(&vm);
    free(code);
    return rc >= 0 ? 0 : 1;
}
