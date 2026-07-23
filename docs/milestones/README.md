# Milestones

Each milestone leaves a VMM that **builds and runs a guest to completion**. Build
them in order (see the [roadmap](../04-roadmap.md) for the dependency graph). Do
one per pass; keep the build green and the guest running.

| # | Milestone | State |
|---|-----------|-------|
| 0 | [KVM hello](milestone-0-kvm-hello.md) | ✅ done |
| 1 | [Serial device model](milestone-1-serial.md) | ⬜ |
| 2 | [Long-mode guest](milestone-2-longmode.md) | ⬜ |
| 3 | [MMIO device](milestone-3-mmio.md) | ⬜ |
| 4 | [ELF64 guest loader](milestone-4-elf-loader.md) | ⬜ |
| 5 | [CPUID + robustness](milestone-5-cpuid-robustness.md) | ⬜ |
| 6 | [Polish](milestone-6-polish.md) | ⬜ |

## Every milestone spec has

- **Goal** — one sentence.
- **Concepts** — what you'll learn.
- **Tasks** — an ordered checklist.
- **Files** — what to add/change.
- **Definition of Done** — the objective bar; don't tick it without running the
  guest and seeing the output.
- **References** — the canonical sources.

## The Builder's loop (from AGENTS.md)

1. Pick the lowest-numbered unfinished milestone.
2. Implement its tasks; keep `make all` clean and `make run` running the guest.
3. Verify against the Definition of Done (actually run it; read the output).
4. Update the concept doc, tick the DoD, update the roadmap and CHANGELOG.
5. Commit (`type(scope): …`), open a PR, keep CI green. Stop for review.
