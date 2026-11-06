# 11: Security boundaries and hardening rationale

nutvisor is an educational, single-VM monitor for trusted demonstration guests.
Parser and device checks reduce accidental corruption; they do **not** make it
a production sandbox for hostile or multi-tenant workloads.

## Research-backed design decisions

- The [System V ELF ABI](https://gabi.xinuos.com/elf/07-pheader.html) defines
  file/memory sizes, zero-fill, segment alignment and ordered load entries.
  nutvisor adds explicit boot constraints: physical and virtual addresses must
  match, load ranges must be disjoint and avoid bootstrap reservations, and the
  entry must lie in file-backed executable bytes. This prevents accepting an
  image that bootstrap setup would immediately corrupt. Header order permits a
  linear overlap check with no additional allocation.
- The [Linux KVM ABI](https://docs.kernel.org/virt/kvm/api.html) describes the
  vCPU mapping size and packed I/O buffers relative to `kvm_run`. The VMM checks
  mapping size, direction, width, count and complete buffer range before pointer
  arithmetic or device callbacks. KVM is part of the trusted host boundary;
  malformed exit metadata tests are defensive contract tests, not evidence of
  a demonstrated kernel escape.
- [Firecracker's design](https://github.com/firecracker-microvm/firecracker/blob/main/docs/design.md)
  separates a small device model from host containment. Its
  [jailer](https://github.com/firecracker-microvm/firecracker/blob/main/docs/jailer.md)
  and [seccomp filtering](https://github.com/firecracker-microvm/firecracker/blob/main/docs/seccomp.md)
  illustrate additional layers absent here. Adding devices or copying its feature
  list would not substitute for those boundaries, so this pass prioritizes input
  contracts and independently testable logic without introducing VMM libraries.

## Safe operation and remaining limits

- Run as an unprivileged user with appropriate KVM access; do not run unfamiliar
  guests as root. Use a disposable host/VM without sensitive files for experiments.
- Run `--check` first. The 128 MiB input cap bounds image-buffer allocation; it is
  not a comprehensive process-memory or CPU quota.
- The guest can loop indefinitely or emit unlimited raw serial/MMIO output,
  including terminal control characters. Capture output to a size-limited file
  rather than displaying unfamiliar guest output directly in a terminal. For
  example, in a disposable shell on a trusted test host:

  ```sh
  (ulimit -f 2048; timeout --kill-after=5s 30s \
      build/nutvisor --elf guests/kernel/kernel.elf >guest.log 2>&1)
  ```

  This is an operational guard, not isolation. Review logs with tools that escape
  control characters. The guest suite uses an external timeout too; a vCPU may
  remain inside `KVM_RUN` without producing exits, so an exit-count limit alone
  would not enforce a deadline.
- The initial 1 GiB identity map is writable/executable. ELF flags do not enforce
  W^X. There are no stack guard pages or bounds enforcement once the guest runs.
  Guests must establish their own page tables, permissions and stack as needed.
- COM1 state is process-global and supports only the current single-VM design.
  Device output errors are not propagated as guest execution failures.
- No seccomp, namespaces, cgroups, privilege-drop jailer or independent security
  audit is provided. The host kernel/KVM and CPU remain trusted dependencies.

## Verification strategy and next work

Host tests check atomic rejection, nonzero-initialized BSS, boundary-touching
ranges, integer-overflow metadata, packed I/O, cleanup and CLI allocation limits.
ASan/UBSan complement these checks. Real KVM boots remain separate and mandatory
for claiming guest execution works; host tests cannot verify CPU mode setup.

Future hardening should add a reviewed process sandbox, signal-driven runtime
cancellation, bounded/error-aware console output, then fuzzing and broader
hardware CI. Each needs its own threat model and regression tests before a change
in supported workload trust is warranted.
