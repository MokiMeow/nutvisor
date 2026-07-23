/* nutvisor entry point: load a guest image from a file and run it. */

#include <stdio.h>
#include <stdlib.h>

#include "vmm.h"

#define GUEST_LOAD_ADDR 0x1000
#define GUEST_MEM_SIZE  (1U << 20) /* 1 MiB of guest RAM */

static long read_file(const char *path, unsigned char **out) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return -1; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) { fclose(f); return -1; }

    unsigned char *buf = malloc((size_t)size);
    if (!buf) { fclose(f); return -1; }
    if (fread(buf, 1, (size_t)size, f) != (size_t)size) {
        free(buf);
        fclose(f);
        return -1;
    }
    fclose(f);
    *out = buf;
    return size;
}

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "build/hello16.bin";
    unsigned char *code;
    long len = read_file(path, &code);
    if (len < 0)
        return 1;

    struct vm vm;
    int rc = 1;
    if (vm_create(&vm, GUEST_MEM_SIZE) == 0
            && vm_load_guest(&vm, GUEST_LOAD_ADDR, code, (size_t)len) == 0
            && vm_set_real_mode(&vm, GUEST_LOAD_ADDR) == 0) {
        fprintf(stderr, "nutvisor: running %s (%ld bytes) as a real-mode guest\n",
                path, len);
        rc = vm_run(&vm);
        if (rc == 0)
            fprintf(stderr, "nutvisor: guest halted cleanly\n");
    }

    vm_destroy(&vm);
    free(code);
    return rc == 0 ? 0 : 1;
}
