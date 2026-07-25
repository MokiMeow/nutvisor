; mmio-demo -- milestone 3 guest
;
; Loaded at guest-physical 0x100000 and entered in 64-bit long mode. The VMM's
; low-GiB identity map makes 0x10000000 accessible, but no RAM slot backs that
; page, so each access exits through KVM_EXIT_MMIO. The guest prints a marker
; one byte at a time and then requests a successful host-side stop.

bits 64
org 0x100000

%define MMIO_BASE 0x10000000

start:
    mov rdi, MMIO_BASE
    mov rsi, message
.next:
    lodsb
    test al, al
    jz .exit
    mov byte [rdi], al
    jmp .next
.exit:
    mov dword [rdi + 8], 0
    hlt                         ; unreachable if the exit device works

message:
    db "nutvisor: mmio console online", 0x0A, 0
