# 09: Testing & debugging

A hypervisor bug usually looks like a guest that silently triple-faults. These
are the techniques that make nutvisor debuggable.

## Read the exit reason

The first question is always "why did `KVM_RUN` return?". nutvisor reports
`SHUTDOWN`, `FAIL_ENTRY`, `INTERNAL_ERROR`, `DEBUG`, `EXCEPTION`, and every
unsupported numeric exit. When something goes wrong, that stderr line is your
first clue:

- **`KVM_EXIT_SHUTDOWN`**: the guest triple-faulted. Its initial CPU state or
  memory is wrong (bad segment, bad page tables, code not where `rip` points).
- **`KVM_EXIT_FAIL_ENTRY`**: the CPU refused to enter the guest; the
  `hardware_entry_failure_reason` is a VT-x code. Almost always an invalid
  `sregs` combination (e.g. long-mode bits set without valid paging).
- **`unsupported KVM exit_reason=N`**: the guest used a device or feature the
  v1 model does not emulate; the following state dump shows where.

## Read the automatic register dump

On a shutdown, failed entry, internal error, debug/exception exit, or
unsupported exit, nutvisor calls `KVM_GET_REGS`/`KVM_GET_SREGS` and prints
`rip`, `rsp`, `rflags`, `cs`, `cr0`, `cr3`, `cr4`, and `efer`. The
`build/fault64.bin` diagnostic guest deliberately executes `ud2`; `make
run-fault` demonstrates this path and is expected to exit nonzero.

## Inspect guest memory directly

Guest-physical memory is just `vm.mem`. After (or during) a run you can dump
bytes at any guest address from the host, with no debugger needed, to confirm your
loader put code where you think, or that the guest wrote what you expect.

## Single-step the guest

KVM supports guest debugging via `KVM_SET_GUEST_DEBUG` (set
`KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_SINGLESTEP`); each instruction then returns a
`KVM_EXIT_DEBUG`. Combined with a register dump per step, this walks the guest
instruction by instruction: invaluable for a mode transition that dies after a
few instructions. (A `-gdb` bridge is a stretch goal.)

## Cross-check against QEMU

If a guest image runs under `qemu-system-x86_64 -kernel/-hda ...` but not under
nutvisor, the bug is in *your* setup, not the guest. QEMU is the oracle.

## Self-test

`make test` runs all five functional guests through KVM and asserts their
markers. It also checks malformed and truncated ELF rejection plus the
deliberate triple-fault register dump. Each run has a 60-second local timeout
(120 seconds in CI), leaving ample cold-start margin while still catching a
hung guest. CI always builds and runs this suite whenever `/dev/kvm` is usable.

## Common failure table

| Symptom | Likely cause |
|---------|--------------|
| `KVM_EXIT_SHUTDOWN` immediately | `rip`/`cs.base` wrong; code not at that address |
| `FAIL_ENTRY` entering long mode | `efer.LMA`/`cr0.PG`/`cr4.PAE`/`cr3` inconsistent |
| Guest prints nothing | serial port mismatch, or guest uses a driver that polls LSR |
| Garbage in guest `.bss` | ELF loader skipped the `p_memsz` zero-fill |
| `open /dev/kvm: No such file` | module not loaded: `./scripts/setup-kvm.sh` |
| `open /dev/kvm: Permission denied` | the device exists but this user cannot open it: `sudo usermod -aG kvm $USER`, then a new shell |

### Existence is not access

The most annoying version of that last row: a check like `[ -e /dev/kvm ]`
succeeds while `open()` still fails with `EACCES`. GitHub's Ubuntu runners are
exactly this case: the device is there, owned by root. This cost a red CI run
before the workflow was fixed. Always test `[ -r /dev/kvm ] && [ -w /dev/kvm ]`
before deciding KVM is available.
