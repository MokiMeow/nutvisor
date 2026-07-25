; fault64 -- milestone 5 diagnostic guest
;
; Loaded at 0x100000 and entered in long mode. UD2 raises #UD with no guest
; IDT installed, deliberately causing a triple fault/KVM_EXIT_SHUTDOWN so the
; VMM's failure-state register dump can be asserted.

bits 64
org 0x100000

start:
    ud2
    hlt
