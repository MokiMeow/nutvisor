# AGENTS.md: how this repo is built

The working agreement for this repository: anyone contributing to nutvisor should
read it fully before making changes. It defines the roles, rules,
build/verify commands, and the milestone path from "runs a real-mode guest" to
"boots a kernel guest." If anything here conflicts with a stray note elsewhere,
**this file wins.**

---

## 1. How the work is organised

- **Planning**: plans the work, defines each milestone and its Definition
  of Done, reviews diffs, keeps docs honest. Decides *what* and *in what order*.
- **Implementation**: proceed one milestone at a time against the spec in
  `docs/milestones/`, keeping the build green and the guest running.

The loop: **pick the lowest-numbered unfinished milestone → implement it →
build clean → run it and prove it with real output → tick its Definition of
Done → update docs/CHANGELOG → commit → stop for review.** One milestone per
pass. (When explicitly told to build the whole project, proceed through all
milestones, still one verified commit at a time.)

## 2. Ground rules (non-negotiable)

1. **The build must never break, and the guest must keep running.** Every commit
   must `make all` cleanly and `make run` must still run milestone 0's guest (or
   the current milestone's guest) to completion.
2. **Warnings are errors in spirit.** Clean under `-Wall -Wextra`. The build uses
   `-std=gnu11` on purpose (POSIX/Linux symbols like `O_CLOEXEC`, `MAP_ANONYMOUS`
   need it); do not switch to strict `-std=c11`.
3. **Prove it, don't assume it.** Kernels and hypervisors fail silently (a guest
   triple-faults into `KVM_EXIT_SHUTDOWN`). Verify by running and checking real
   output, never by "it should work."
4. **New `.c` → `src/`, new `.h` → `include/`, new guest → `guests/`.** The
   Makefile auto-discovers `src/*.c`; add guest build rules explicitly.
5. **No dependencies.** Only libc and the Linux headers (`<linux/kvm.h>`). No
   external VMM libraries: the point is to write it.
6. **No feature without a doc.** When a milestone adds a capability (long mode,
   MMIO, ELF loading), its concept doc in `docs/` must be accurate when it lands.
7. **Small, honest commits** (see §5).

## 3. Build, run, verify

Run from the repo root on WSL2 / Linux. The environment needs `/dev/kvm`
(nested virtualization). `make run` loads the module automatically.

| Command | What it does |
|---------|--------------|
| `./scripts/setup-kvm.sh` | Ensure `/dev/kvm` exists (loads the KVM module). |
| `make all` | Build `build/nutvisor` and the guest image(s). |
| `make test` | Run the complete guest and failure-path regression suite. |
| `make run` | Build, ensure `/dev/kvm`, and run the current guest. |
| `make clean` | Remove `build/`. |

**Definition of "it works" for any change:**
1. `make clean && make all` builds with no warnings.
2. `make run` runs the guest to completion and prints the expected output.
3. `make test` passes every success and deliberate-failure assertion (this is
   what `.github/workflows/ci.yml` runs where KVM is usable).

**A trap worth knowing:** GitHub's Ubuntu runners *do* ship `/dev/kvm`, but
root-only, so testing `[ -e /dev/kvm ]` passes and the guest then fails with
`Permission denied`. Always check `[ -r /dev/kvm ] && [ -w /dev/kvm ]`. CI
installs a udev rule to open the device up, runs the guest for real, and only
skips if the device is genuinely unusable.

Never claim a milestone is done without having actually run its guest.

## 4. Coding standards

- **C**: C (gnu11), 4-space indent, K&R braces, `snake_case`, `UPPER_CASE` for
  macros. Check every `ioctl`/`mmap`/`open` return value and `perror` on
  failure: VMM bugs are almost always an unchecked syscall.
- **Structure**: keep KVM lifecycle in `vm.c`, one device per file
  (`serial.c`, later `mmio.c`, etc.), guest loading separate from running.
- **Assembly/guests**: NASM, `-f bin` for flat real-mode blobs, ELF for kernel
  guests. Each guest has a comment block stating its load address and what it
  does.
- Comment *why* (which VM exit, which CPU mode, which spec quirk), not *what*.

## 5. Commit and branch style

- Format: `type(scope): outcome`, imperative, lower case.
  Examples: `feat(longmode): bring the guest up into 64-bit mode`,
  `fix(serial): return LSR ready so the guest's driver polls correctly`.
- Types: `feat`, `fix`, `docs`, `refactor`, `build`, `chore`, `test`.
- **No AI/co-author trailers.** Commits are authored by the repo owner.
- Branch per milestone (`milestone-2-longmode`), PR into `main`, keep CI green.

## 6. The milestone path (what "finished" means)

Full specs with Definition of Done live in `docs/milestones/`.

| # | Milestone | Adds | Spec |
|---|-----------|------|------|
| 0 | KVM hello | VM/vCPU/memory, real-mode guest, serial + HLT exits | [spec](docs/milestones/milestone-0-kvm-hello.md) ✅ |
| 1 | Serial | port-I/O dispatch table, fuller 16550, guest-from-file | [spec](docs/milestones/milestone-1-serial.md) ✅ |
| 2 | Long mode | GDT + page tables in guest memory, 64-bit sregs | [spec](docs/milestones/milestone-2-longmode.md) ✅ |
| 3 | MMIO | `KVM_EXIT_MMIO` handling + a memory-mapped device | [spec](docs/milestones/milestone-3-mmio.md) ✅ |
| 4 | ELF loader | parse ELF64, load PT_LOAD segments, jump to entry | [spec](docs/milestones/milestone-4-elf-loader.md) ✅ |
| 5 | CPUID/robustness | `KVM_SET_CPUID2`, clean handling of every exit reason | [spec](docs/milestones/milestone-5-cpuid-robustness.md) ✅ |
| 6 | Polish | self-test, CI, demo, tag `v1.0.0` | [spec](docs/milestones/milestone-6-polish.md) ✅ |

**Definition of Done (whole project):** nutvisor boots a 64-bit ELF guest kernel
that runs and produces output through the emulated devices. Stretch goal: boot
the real Nutshell kernel via multiboot2 emulation.

## 7. What NOT to do

- Do not link an existing VMM/emulator library; write the KVM calls yourself.
- Do not skip the run/verify step because "the ioctls look right."
- Do not switch to `-std=c11` (breaks POSIX symbols) or add `-Werror`-defeating
  pragmas: fix the cause.
- Do not assume `/dev/kvm` persists across a WSL restart; `make run` reloads it.

## 8. Tools reference

- **gcc**: host C compiler (`-std=gnu11`).
- **NASM**: assembles guest blobs (`-f bin` for real-mode, ELF for kernels).
- **Linux KVM**: `<linux/kvm.h>` (from the installed kernel headers) is the
  whole API surface; there is nothing else to install.
- **`/dev/kvm`**: the device the whole project talks to. `make run` ensures it.

Build one milestone, run its guest, prove it with output, document it, commit.
Then the next.
