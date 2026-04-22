[BITS 32]
section .text
GLOBAL i386context_switch
GLOBAL i386context_load

; void i386context_switch(context_t *old, context_t new);
; cdecl:
;   [esp + 4]  = old (pointer to context_t)
;   [esp + 8]  = new (context_t)

i386context_switch:
    ; stack layout:
    ; [esp + 4] = old_ctx
    ; [esp + 8] = new_ctx

    pushfd              ; save EFLAGS
    pushad              ; save general registers

    mov eax, [esp + 36] ; old_ctx (after pushad+pushfd = +32+4)
    test eax, eax
    jz .skip_save

    mov [eax], esp      ; *old_ctx = current ESP

.skip_save:
    mov eax, [esp + 40] ; new_ctx
    mov esp, eax        ; switch stack

    popad               ; restore registers
    popfd               ; restore flags

    ret                 ; jump to new thread
