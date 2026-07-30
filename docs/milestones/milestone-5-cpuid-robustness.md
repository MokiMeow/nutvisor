# Milestone 5: CPUID + robustness ✅ (done)

**Goal:** make the VMM robust enough to host a real kernel: a sane CPUID, and
clean handling of every VM-exit reason.

## Concepts

Guest CPUID (`KVM_GET_SUPPORTED_CPUID` / `KVM_SET_CPUID2`), why a kernel guest
queries CPUID early, and completing the VM-exit switch so nothing is "unhandled."

## Tasks

- [x] Set the guest's CPUID: fetch `KVM_GET_SUPPORTED_CPUID` from the KVM fd and
      apply it with `KVM_SET_CPUID2` on the vCPU, so the guest sees a coherent
      feature set (kernels branch on CPUID during early boot).
- [x] Audit the exit switch in `vm.c`: handle or explicitly, deliberately report
      every reason a kernel guest can produce (`IO`, `MMIO`, `HLT`, `SHUTDOWN`,
      `FAIL_ENTRY`, `INTERNAL_ERROR`, `DEBUG` if single-stepping). No silent
      `default`.
- [x] Improve diagnostics: on `SHUTDOWN`/`FAIL_ENTRY`, dump key registers
      (`rip`, `cs`, `cr0/cr3/cr4`, `efer`) to make triple faults debuggable.
- **Deferred optional (post-v1):** restart guest state after a reset request
  instead of reporting the exit and stopping.

## Files

`src/cpuid.c` + `include/cpuid.h` (or fold into `vm.c`), `src/vm.c`
(exit-switch completeness + register dumps).

## Definition of Done

- [x] The guest kernel from milestone 4 boots with CPUID set, exercising more of
      its early-boot path than before.
- [x] Every exit reason is handled or reported with useful detail; a triple
      fault prints a register dump, not just "guest shutdown."
- [x] `make all` clean; earlier guests still run.

## References

- Linux KVM API: `KVM_SET_CPUID2`, `KVM_GET_SUPPORTED_CPUID`
- [docs/09: Testing & debugging](../09-testing-and-debugging.md)

**Next:** [Milestone 6: Polish](milestone-6-polish.md).
