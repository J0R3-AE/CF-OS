[BITS 32]
section .text
GLOBAL i386context_switch
GLOBAL i386context_load

; void i386context_switch(context_t *old, context_t new);
; cdecl:
;   [esp + 4]  = old (pointer to context_t)
;   [esp + 8]  = new (context_t)

i386context_switch:
    ; save current context on current stack
    pushfd              ; EFLAGS
    pushad              ; EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI

    ; esp now points at saved EDI (top of pushad/pushfd frame)
    ; args are now at esp + 36 (old) and esp + 40 (new)

    mov     eax, [esp + 36]     ; eax = &old
    mov     [eax], esp          ; *old = current esp (saved frame)

    mov     eax, [esp + 40]     ; eax = new
    mov     esp, [eax]          ; esp = new saved frame

    ; restore new context
    popad                       ; restore regs
    popfd                       ; restore flags
    ret
