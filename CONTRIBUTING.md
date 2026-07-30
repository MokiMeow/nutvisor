# Contributing to nutvisor

This is primarily a learning/portfolio project, but clean contributions are
welcome.

## Before you start

- Read [AGENTS.md](AGENTS.md): the operating manual, which applies to humans
  too.
- Skim [docs/00-overview.md](docs/00-overview.md) and the
  [roadmap](docs/04-roadmap.md).
- Make sure `/dev/kvm` exists (`./scripts/setup-kvm.sh`).

## Workflow

1. Pick the lowest-numbered unfinished milestone in
   [docs/milestones/](docs/milestones/).
2. Branch from `main`: `git checkout -b milestone-2-longmode`.
3. Implement it. Keep the build green (`make clean && make all`) and the full
   guest regression suite passing (`make test`) at every commit.
4. Update the relevant doc and tick the Definition of Done.
5. Open a PR into `main`. CI must be green (CI builds always; it runs the guest
   only where `/dev/kvm` exists).

## Commit style

`type(scope): outcome` in the imperative, lower case. Types: `feat`, `fix`,
`docs`, `refactor`, `build`, `chore`, `test`. No AI/co-author trailers.

Example: `feat(mmio): emulate a memory-mapped debug console`.

## Code style

See §4 of [AGENTS.md](AGENTS.md). Short version: C (gnu11), 4-space indent,
`snake_case`, check every syscall return and `perror` on failure, one device per
file, comment *why* not *what*.

## Reporting issues

Say what you did, what you expected, and what happened. For guest failures,
include the VMM's stderr (it reports the `KVM_EXIT_*` reason) and your host /
`/dev/kvm` status.
