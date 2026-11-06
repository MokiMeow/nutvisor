/* nutvisor entry point: load a guest image from a file and run it. */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loader.h"
#include "memory_layout.h"
#include "vmm.h"

#define REAL_GUEST_LOAD_ADDR 0x1000ULL
#define LONG_GUEST_LOAD_ADDR 0x100000ULL
#define GUEST_MEM_SIZE       (64U << 20) /* 64 MiB of guest RAM */
#define MAX_IMAGE_SIZE       (128U << 20) /* Includes ELF debug/file metadata. */

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
    /* Bound host allocation before reading even a sparse or padded image. */
    if ((unsigned long)size > MAX_IMAGE_SIZE) {
        fprintf(stderr, "%s: guest image exceeds the 128 MiB file limit\n", path);
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

static void usage(FILE *stream, const char *name) {
    fprintf(stream,
            "usage: %s [--check] [--real|--long|--elf] [--] [guest-image]\n"
            "  --check  validate the image without opening /dev/kvm\n"
            "  --help   show this help\n", name);
}

int main(int argc, char **argv) {
    const char *path = "guests/kernel/kernel.elf";
    enum guest_mode mode = GUEST_MODE_ELF;
    bool check_only = false;
    bool explicit_mode = false;
    int arg = 1;

    for (; arg < argc && argv[arg][0] == '-'; arg++) {
        const char *option = argv[arg];
        if (strcmp(option, "--") == 0) {
            arg++;
            break;
        }
        if (strcmp(option, "--help") == 0) {
            usage(stdout, argv[0]);
            return 0;
        }
        if (strcmp(option, "--check") == 0 && !check_only) {
            check_only = true;
        } else if (!explicit_mode && (strcmp(option, "--real") == 0
                    || strcmp(option, "--long") == 0
                    || strcmp(option, "--elf") == 0)) {
            explicit_mode = true;
            mode = strcmp(option, "--elf") == 0 ? GUEST_MODE_ELF
                : strcmp(option, "--long") == 0 ? GUEST_MODE_LONG
                : GUEST_MODE_REAL;
        } else {
            usage(stderr, argv[0]);
            return 2;
        }
    }
    if (argc - arg > 1 || (explicit_mode && arg == argc)) {
        usage(stderr, argv[0]);
        return 2;
    }
    if (arg < argc) {
        path = argv[arg];
        if (!explicit_mode)
            mode = has_suffix(path, ".elf") ? GUEST_MODE_ELF : GUEST_MODE_REAL;
    }

    uint64_t load_addr = mode == GUEST_MODE_LONG
        ? LONG_GUEST_LOAD_ADDR : REAL_GUEST_LOAD_ADDR;
    unsigned char *code;
    long len = read_file(path, &code);
    if (len < 0)
        return 1;

    uint64_t entry = load_addr;
    int valid;
    if (mode == GUEST_MODE_ELF) {
        valid = elf64_validate(code, (size_t)len, GUEST_MEM_SIZE, &entry);
    } else {
        valid = vm_range_fits(load_addr, (uint64_t)len, GUEST_MEM_SIZE)
            && (mode != GUEST_MODE_LONG
                || !vm_boot_range_reserved(load_addr, (uint64_t)len)) ? 0 : -1;
        if (valid < 0)
            fprintf(stderr, "guest image exceeds RAM or overlaps bootstrap memory\n");
    }
    if (valid < 0) {
        free(code);
        return 1;
    }
    if (check_only) {
        printf("nutvisor: image valid, entry=0x%llx\n",
               (unsigned long long)entry);
        free(code);
        return 0;
    }

    struct vm vm;
    int rc = VM_RUN_ERROR;
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
