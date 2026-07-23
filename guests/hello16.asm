; hello16 — the milestone 0 guest.
;
; A 16-bit real-mode program that the VMM loads at guest-physical 0x1000 and
; runs. It writes a string to COM1 (port 0x3F8) one byte at a time — each OUT
; causes a KVM_EXIT_IO that the host VMM turns into a character on stdout — and
; then HLTs, which the VMM sees as KVM_EXIT_HLT and stops.

bits 16
org 0x1000

start:
    mov dx, 0x3F8          ; COM1 data port
    mov si, message
.next:
    lodsb                  ; al = [ds:si], si++
    test al, al
    jz .done
    out dx, al             ; -> KVM_EXIT_IO (host prints al)
    jmp .next
.done:
    hlt                    ; -> KVM_EXIT_HLT (host stops the guest)

message:
    db "nutvisor: the guest is alive inside your hypervisor", 0x0A, 0
