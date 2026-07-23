# nutvisor documentation

Start here. These docs explain what nutvisor is and how to build it, step by
step, up to booting a kernel guest.

## Read in this order

1. [00 — Overview](00-overview.md) — the big picture and design goals.
2. [01 — Getting started](01-getting-started.md) — KVM/nested-virt setup, build, run.
3. [02 — Architecture](02-architecture.md) — how the pieces fit together.
4. [03 — The KVM API](03-kvm-api.md) — ioctls, guest memory, the VM-exit loop.
5. [04 — Roadmap](04-roadmap.md) — the milestone plan to a kernel guest.

## Concept references (per subsystem)

- [05 — Guest memory](05-guest-memory.md)
- [06 — vCPU & CPU modes](06-vcpu-and-modes.md)
- [07 — Device emulation](07-device-emulation.md)
- [08 — ELF loading](08-elf-loading.md)
- [09 — Testing & debugging](09-testing-and-debugging.md)
- [10 — Glossary](10-glossary.md)

## Milestones

Step-by-step specs, each with a Definition of Done, live in
[milestones/](milestones/).

## Design decisions

The *why* behind the big choices lives in [decisions/](decisions/) (ADRs).
