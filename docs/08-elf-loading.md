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

Use `p_paddr` (physical) to place segments, because at entry the guest usually
hasn't set up its own virtual mapping yet. `p_vaddr` matters once the guest's
page tables are live. For an identity-mapped low kernel they're equal.

## Validation (implemented, before any guest RAM is changed)

A malformed or hostile ELF can point segments anywhere. Check:
- the magic (`\x7f ELF`), 64-bit class, the machine is x86-64,
- every `p_offset + p_filesz` is within the file,
- every `p_paddr + p_memsz` is within guest memory,
- `p_memsz >= p_filesz`,
- `e_entry` lands inside an executable loaded segment,
- the ELF and program-header layouts and versions are supported.

`src/loader.c` uses subtraction-based bounds checks to avoid integer-overflow
bypasses and copies potentially unaligned headers into local structs. The
validation pass completes before a second pass copies and zero-fills segments.
Malformed images fail with an `ELF: ...` diagnostic.

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

- ELF-64 specification (program header table)
- `man 5 elf`
