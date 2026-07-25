/* nutvisor entry point: load a guest image from a file and run it. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vmm.h"

#define REAL_GUEST_LOAD_ADDR 0x1000ULL
#define LONG_GUEST_LOAD_ADDR 0x100000ULL
#define GUEST_MEM_SIZE       (64U << 20) /* 64 MiB of guest RAM */

enum guest_mode {
    GUEST_MODE_REAL,
    GUEST_MODE_LONG,
};

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
    const char *path = "build/hello64.bin";
    enum guest_mode mode = GUEST_MODE_LONG;

    if (argc == 2) {
        /* Preserve the milestone-0 interface for explicit flat binaries. */
        path = argv[1];
        mode = GUEST_MODE_REAL;
    } else if (argc == 3
            && (strcmp(argv[1], "--real") == 0
                || strcmp(argv[1], "--long") == 0)) {
        mode = strcmp(argv[1], "--long") == 0
            ? GUEST_MODE_LONG : GUEST_MODE_REAL;
        path = argv[2];
    } else if (argc != 1) {
        fprintf(stderr, "usage: %s [--real|--long] [guest-image]\n", argv[0]);
        return 2;
    }

    uint64_t load_addr = mode == GUEST_MODE_LONG
        ? LONG_GUEST_LOAD_ADDR : REAL_GUEST_LOAD_ADDR;
    unsigned char *code;
    long len = read_file(path, &code);
    if (len < 0)
        return 1;

    struct vm vm;
    int rc = 1;
    if (vm_create(&vm, GUEST_MEM_SIZE) == 0
            && vm_load_guest(&vm, load_addr, code, (size_t)len) == 0
            && (mode == GUEST_MODE_LONG
                ? vm_set_long_mode(&vm, load_addr)
                : vm_set_real_mode(&vm, load_addr)) == 0) {
        fprintf(stderr, "nutvisor: running %s (%ld bytes) in %s mode\n",
                path, len, mode == GUEST_MODE_LONG ? "64-bit long" : "16-bit real");
        rc = vm_run(&vm);
        if (rc == 0)
            fprintf(stderr, "nutvisor: guest halted cleanly\n");
    }

    vm_destroy(&vm);
    free(code);
    return rc == 0 ? 0 : 1;
}
