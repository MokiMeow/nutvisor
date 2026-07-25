; serial-driver -- milestone 1 guest
;
; Loaded at guest-physical 0x1000 in 16-bit real mode. It configures COM1 like
; a real polling UART driver, waits for THR-ready in the LSR, prints, and halts.

bits 16
org 0x1000

%define COM1 0x3F8

start:
    mov dx, COM1 + 1
    xor al, al
    out dx, al                  ; disable interrupts

    mov dx, COM1 + 3
    mov al, 0x80
    out dx, al                  ; set DLAB

    mov dx, COM1
    mov al, 0x03
    out dx, al                  ; divisor low: 38400 baud
    mov dx, COM1 + 1
    xor al, al
    out dx, al                  ; divisor high

    mov dx, COM1 + 3
    mov al, 0x03
    out dx, al                  ; 8 data bits, no parity, one stop
    mov dx, COM1 + 2
    mov al, 0xC7
    out dx, al                  ; enable and clear FIFOs
    mov dx, COM1 + 4
    mov al, 0x0B
    out dx, al                  ; IRQs, RTS, DTR

    mov si, message
.next:
    lodsb
    test al, al
    jz .done
    mov bl, al
.wait:
    mov dx, COM1 + 5
    in al, dx
    test al, 0x20              ; transmitter holding register empty
    jz .wait
    mov al, bl
    mov dx, COM1
    out dx, al
    jmp .next
.done:
    hlt

message:
    db "nutvisor: 16550 driver online", 0x0A, 0
