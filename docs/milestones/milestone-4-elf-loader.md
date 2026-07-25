# Milestone 4 — ELF64 guest loader ✅ (done)

**Goal:** load a 64-bit ELF kernel image from a file into guest memory and boot
it — the "I wrote the VM and the OS it runs" milestone.

## Concepts

The ELF64 header and program headers, `PT_LOAD` segments (`p_offset`,
`p_filesz`, `p_memsz`, `p_paddr`), the `.bss` zero-fill, and entering at
`e_entry`.

## Tasks

- [x] `src/loader.c` + `include/loader.h`: parse an ELF64 file — validate the
      magic, 64-bit class, and x86-64 machine; iterate program headers.
- [x] For each `PT_LOAD`: `memcpy` `p_filesz` bytes to `mem + p_paddr`, then
      zero-fill up to `p_memsz` (the guest's `.bss`).
- [x] Validate every segment fits in guest memory and `e_entry` lands in a
      loaded segment; refuse malformed images.
- [x] Bring the guest up in long mode (milestone 2) with `rip = e_entry`.
- [x] Write a small 64-bit ELF **guest kernel** (`guests/kernel/`) that prints
      via the serial device and halts, and boot it through the loader.

## Files

`src/loader.c` + `include/loader.h`, `src/main.c` (ELF vs. flat-binary guests),
`guests/kernel/*` (a small 64-bit ELF guest + its linker script/Makefile rule).

## Definition of Done

- [x] `./build/nutvisor guests/kernel/kernel.elf` loads and runs the guest
      kernel, which prints its output through the emulated serial and halts.
- [x] Malformed ELF inputs are rejected with a clear message, not a crash.
- [x] `make all` clean; earlier guests still run.

## References

- ELF-64 specification; `man 5 elf`
- [docs/08 — ELF loading](../08-elf-loading.md)

**Next:** [Milestone 5 — CPUID + robustness](milestone-5-cpuid-robustness.md).
