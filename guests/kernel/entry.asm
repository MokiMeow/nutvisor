; kernel/entry -- milestone 4 ELF64 guest kernel
;
; Linked at guest-physical/virtual 0x100000 and entered directly in long mode.
; The VMM supplies identity paging, flat segments, a stack, and emulated COM1.
; The .bss probe proves the ELF loader performs the required zero-fill.

bits 64
default abs

section .text
global _start

%define COM1 0x3F8

_start:
    xor eax, eax
    cpuid
    test eax, eax
    jz .cpuid_failed
    mov rsi, cpuid_message
    call serial_print

    cmp qword [bss_probe], 0
    jne .bss_failed
    mov rsi, message
    call serial_print
    hlt

.bss_failed:
    mov rsi, bss_failure
    call serial_print
    mov rdi, 0x10000008
    mov dword [rdi], 1
    hlt

.cpuid_failed:
    mov rsi, cpuid_failure
    call serial_print
    mov rdi, 0x10000008
    mov dword [rdi], 2
    hlt

serial_print:
    lodsb
    test al, al
    jz .done
    mov bl, al
.wait:
    mov dx, COM1 + 5
    in al, dx
    test al, 0x20
    jz .wait
    mov al, bl
    mov dx, COM1
    out dx, al
    jmp serial_print
.done:
    ret

section .rodata
cpuid_message:
    db "nutvisor: cpuid online", 0x0A, 0
message:
    db "nutvisor: elf64 kernel online", 0x0A, 0
bss_failure:
    db "nutvisor: ELF bss zero-fill failed", 0x0A, 0
cpuid_failure:
    db "nutvisor: CPUID setup failed", 0x0A, 0

section .bss
align 8
bss_probe:
    resq 1
