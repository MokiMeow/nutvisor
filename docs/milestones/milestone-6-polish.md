# Milestone 6 — Polish (portfolio pass)

**Goal:** turn a working hypervisor into a repo that impresses on sight —
proof, presentation, repeatability — and tag `v1.0.0`.

## Tasks

### Proof it works
- [ ] Add a self-test: run a guest that writes a known string (or a known code
      to the milestone-3 exit device), capture stdout, and assert it. Wire it
      into a `make test` target that exits non-zero on mismatch.
- [ ] Ensure CI runs `make test` where `/dev/kvm` exists and always runs
      `make all` (build) everywhere. Report a KVM-absent runner as a skip, not a
      failure.

### Presentation
- [ ] Record a short demo (asciinema or a GIF) of a guest — ideally the
      milestone-4 guest kernel — booting inside nutvisor and printing output.
      Embed it at the top of the README.
- [ ] Add an architecture diagram / "how a VM exit flows" section to the README
      (link to [docs/03](../03-kvm-api.md)).

### Hygiene
- [ ] Every doc's status table accurate; all milestone Definitions of Done ticked.
- [ ] `CHANGELOG.md` updated; items moved from Unreleased to `1.0.0`.
- [ ] Build is warning-free; CI green on `main`.
- [ ] Tag the release: `git tag v1.0.0`.

## Definition of Done

- [ ] CI builds everywhere and runs the guest self-test where KVM exists, green
      on `main`.
- [ ] README opens with the demo and a crisp description.
- [ ] `v1.0.0` tagged. The repo reads as finished, not abandoned.

## Stretch goal (the headline) — boot Nutshell

Attempt to boot the real Nutshell kernel (`../nutshell`) as the guest by
emulating the multiboot2 hand-off: load its ELF, build a multiboot2 info
structure in guest memory, set `eax` to the multiboot2 magic and `ebx` to the
info pointer, and enter 32-bit protected mode at the kernel's entry. Document the
result whether or not it fully boots — the attempt and findings are themselves
worth showing. Ship `v1.0.0` first; this is a bonus.
