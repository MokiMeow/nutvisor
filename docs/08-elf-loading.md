# 08: ELF loading

*Milestone 4.* Loading a kernel guest means parsing an ELF64 file and copying
its segments into guest-physical memory, then entering at its entry point.

## What an ELF loader does

An ELF64 file has a header (`Elf64_Ehdr`) and an array of **program headers**
(`Elf64_Phdr`). The loader only needs the program headers of type `PT_LOAD`:
each says "copy `p_filesz` bytes from file offset `p_offset` to
address `p_paddr`, then zero-fill up to `p_memsz`."

```c
for each phdr with p_type == PT_LOAD:
    memcpy(mem + phdr.p_paddr, file + phdr.p_offset, phdr.p_filesz);
    memset(mem + phdr.p_paddr + phdr.p_filesz, 0, phdr.p_memsz - phdr.p_filesz);
set rip = ehdr.e_entry;
```

The `p_memsz > p_filesz` zero-fill is the guest's `.bss`; forgetting it leaves
garbage where the kernel expects zeros: a classic, silent bug.

## Physical vs. virtual addresses

nutvisor places segments at `p_paddr` and requires `p_vaddr == p_paddr`:
its initial page tables are an identity map. Higher-half kernels need a different
boot protocol and are rejected instead of silently starting at an unmapped entry.

## Validation (implemented, before any guest RAM is changed)

A malformed or hostile ELF can point segments anywhere. Check:
- the magic (`\x7f ELF`), 64-bit class, the machine is x86-64,
- every `p_offset + p_filesz` is within the file,
- every `p_paddr + p_memsz` is within guest memory,
- `p_memsz >= p_filesz`,
- `e_entry` lands inside **file-backed** executable bytes, not only zero-filled BSS,
- nonempty loadable segments are ordered by address and do not overlap,
- `p_align` is 0, 1, or a power of two with congruent virtual/file offsets,
- no segment overlaps the bootstrap tables or initial downward-growing stack,
- no `PT_INTERP` or `PT_DYNAMIC` requests unsupported dynamic linking,
- the ELF and program-header layouts and versions are supported.

`src/loader.c` uses subtraction-based bounds checks to avoid integer-overflow
bypasses and copies potentially unaligned headers into local structs. The
validation pass completes before a second pass copies and zero-fills segments.
Malformed or unsupported images fail with an `ELF: ...` diagnostic. Ordering
allows overlap checks in O(program-header count), without allocating a segment
index. At most 1024 program headers are accepted. Zero-sized segments and harmless
metadata such as `PT_NOTE` are accepted, but cannot supply executable entry bytes.

These are nutvisor's **boot protocol restrictions**, not a claim that all other
ELF layouts are invalid. In particular, dynamic executables, overlapping load
segments, higher-half images and BSS entry points are intentionally unsupported.
`p_flags` checks entry eligibility; the bootstrap identity map does not enforce
per-segment read/write/execute permissions.

The shared layout in `include/memory_layout.h` reserves `[0x2000, 0x6000)` for
page tables/GDT and `[0x7f0000, 0x800000)` for the 64 KiB bootstrap stack. A segment
may end exactly at a reserved range's start, or start exactly at its end. Guests
needing a larger stack must switch to their own safe stack before exhausting it.

`elf64_validate` checks an image without a VM or RAM allocation. `elf64_load`
revalidates before copying and leaves both guest RAM and the entry output unchanged
on failure. The caller must keep the input image separate from guest RAM.

The CLI caps the complete input file at 128 MiB before allocation, independently
of the 64 MiB guest RAM limit. This allows ELF metadata/debug sections while
preventing unbounded allocation from huge or sparse files. Validation runs before
opening `/dev/kvm`, and `build/nutvisor --check --elf kernel.elf` performs preflight
only. This establishes loadability, not successful guest execution.

## Entering the kernel

After loading, set the vCPU to the mode the kernel's entry expects:
- a **64-bit ELF kernel**: bring up long mode (see [docs/06](06-vcpu-and-modes.md))
  and set `rip = e_entry`;
- a **multiboot2 kernel** (the Nutshell stretch goal): enter 32-bit protected
  mode at `e_entry` with `eax` = the multiboot2 magic and `ebx` pointing at a
  multiboot2 info structure you build in guest memory.

The v1 guest is assembled from `guests/kernel/entry.asm` and linked at
`0x100000` by `guests/kernel/linker.ld`. Its `.bss` probe must be zero before it
prints `nutvisor: elf64 kernel online`, directly exercising the loader's
zero-fill path. `make run` builds and boots `guests/kernel/kernel.elf`.

## References

- [System V ABI: program headers and loading](https://gabi.xinuos.com/elf/07-pheader.html)
- [Hardening rationale and security boundaries](11-security.md)
