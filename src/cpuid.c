/* Install KVM's supported CPUID table on the vCPU so guest feature discovery
 * is coherent with the virtualization backend. */

#include <errno.h>
#include <linux/kvm.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "cpuid.h"
#include "vmm.h"

#define INITIAL_CPUID_ENTRIES 128U
#define MAX_CPUID_ENTRIES     4096U

int cpuid_setup(struct vm *vm) {
    uint32_t entries = INITIAL_CPUID_ENTRIES;
    struct kvm_cpuid2 *cpuid = NULL;

    for (;;) {
        size_t size = sizeof(*cpuid)
            + (size_t)entries * sizeof(struct kvm_cpuid_entry2);
        cpuid = calloc(1, size);
        if (!cpuid) {
            perror("calloc CPUID table");
            return -1;
        }
        cpuid->nent = entries;

        if (ioctl(vm->kvm_fd, KVM_GET_SUPPORTED_CPUID, cpuid) == 0)
            break;
        if (errno != E2BIG) {
            perror("KVM_GET_SUPPORTED_CPUID");
            free(cpuid);
            return -1;
        }

        uint32_t required = cpuid->nent;
        free(cpuid);
        cpuid = NULL;
        if (required <= entries)
            required = entries * 2U;
        if (required > MAX_CPUID_ENTRIES) {
            fprintf(stderr, "KVM requested an unreasonable CPUID table\n");
            return -1;
        }
        entries = required;
    }

    if (ioctl(vm->vcpu_fd, KVM_SET_CPUID2, cpuid) < 0) {
        perror("KVM_SET_CPUID2");
        free(cpuid);
        return -1;
    }
    fprintf(stderr, "nutvisor: installed %u CPUID entries\n", cpuid->nent);
    free(cpuid);
    return 0;
}
