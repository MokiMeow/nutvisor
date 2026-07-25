; hello64 -- milestone 2 guest
;
; Loaded at guest-physical 0x100000. The VMM enters with long mode active,
; low memory identity-mapped, a flat 64-bit code segment, and rsp=0x800000.
; This driver-style guest polls COM1, prints a marker, and halts.

bits 64
org 0x100000

%define COM1 0x3F8

start:
    mov rsi, message
.next:
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
    jmp .next
.done:
    hlt

message:
    db "nutvisor: long mode online", 0x0A, 0
