/* Host-only KVM I/O span tests: no /dev/kvm required. */
#include <stdint.h>
#include <stdio.h>
#include "io_bounds.h"

#define EXPECT(expression) do { if (!(expression)) return __LINE__; } while (0)
int main(void) {
    EXPECT(vm_io_span_valid(4096, 0, 1, 1));
    EXPECT(vm_io_span_valid(4096, 4080, 4, 4));
    EXPECT(vm_io_span_valid(4096, 4095, 1, 1));
    EXPECT(!vm_io_span_valid(4096, 4081, 4, 4));
    EXPECT(!vm_io_span_valid(4096, 4096, 1, 1));
    EXPECT(!vm_io_span_valid(4096, 4097, 1, 1));
    EXPECT(!vm_io_span_valid(4096, UINT64_MAX, UINT32_MAX, 4));
    EXPECT(!vm_io_span_valid(4096, 0, UINT32_MAX, 4));
    EXPECT(!vm_io_span_valid(4096, 0, 0, 1));
    EXPECT(!vm_io_span_valid(4096, 0, 1, 0));
    EXPECT(!vm_io_span_valid(4096, 0, 1, 8));
    EXPECT(!vm_io_span_valid(0, 0, 1, 1));
    puts("ok KVM I/O span bounds");
    return 0;
}
