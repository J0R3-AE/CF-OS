BITS 32

section .text

global i386_context_switch
global i386_context_create

; ---------------------------------------------------------------------------
; i386 saved context frame
;
; After:
;
;     pushfd
;     pushad
;
; ESP points at:
;
;     +00  EDI
;     +04  ESI
;     +08  EBP
;     +12  ESP (dummy/original value)
;     +16  EBX
;     +20  EDX
;     +24  ECX
;     +28  EAX
;     +32  EFLAGS
;     +36  return address
;     +40  bootstrap argument
;     +44  entry function
;
; ---------------------------------------------------------------------------


; ---------------------------------------------------------------------------
; void i386_context_switch(
;     ArchContext *old_context,
;     const ArchContext *new_context
; );
;
; cdecl:
;
;     [esp + 4] = old_context
;     [esp + 8] = new_context
;
; ---------------------------------------------------------------------------

i386_context_switch:

    ; Save current CPU state.
    pushfd
    pushad

    ; Save current ESP into old_context->esp.
    mov eax, [esp + 36]
    mov [eax], esp

    ; Load new context ESP.
    mov eax, [esp + 40]
    mov esp, [eax]

    ; Restore incoming CPU state.
    popad
    popfd

    ; Resume incoming context.
    ret


; ---------------------------------------------------------------------------
; void i386_context_create(
;     ArchContext *context,
;     void (*entry)(void *),
;     void *arg,
;     void *stack_top
; );
;
; cdecl:
;
;     [esp + 4]  = context
;     [esp + 8]  = entry
;     [esp + 12] = arg
;     [esp + 16] = stack_top
;
; ---------------------------------------------------------------------------

i386_context_create:

    mov eax, [esp + 4]      ; context
    mov edx, [esp + 8]      ; entry
    mov ecx, [esp + 12]     ; arg
    mov ebx, [esp + 16]     ; stack_top

    ; Align stack down to 16 bytes.
    and ebx, 0xFFFFFFF0

    ; -----------------------------------------------------------------------
    ; Build:
    ;
    ;     popad
    ;     popfd
    ;     ret
    ;
    ; followed by:
    ;
    ;     arg
    ;     entry
    ;
    ; -----------------------------------------------------------------------

    ; Entry address returned to after popfd.
    sub ebx, 4
    mov dword [ebx], bootstrap_entry

    ; EFLAGS
    sub ebx, 4
    mov dword [ebx], 0x202

    ; POPAD frame:
    ;
    ; EDI
    ; ESI
    ; EBP
    ; ESP dummy
    ; EBX
    ; EDX
    ; ECX
    ; EAX

    sub ebx, 4
    mov dword [ebx], 0        ; EDI

    sub ebx, 4
    mov dword [ebx], 0        ; ESI

    sub ebx, 4
    mov dword [ebx], 0        ; EBP

    sub ebx, 4
    mov dword [ebx], 0        ; ESP dummy

    sub ebx, 4
    mov dword [ebx], 0        ; EBX

    sub ebx, 4
    mov dword [ebx], 0        ; EDX

    sub ebx, 4
    mov dword [ebx], 0        ; ECX

    sub ebx, 4
    mov dword [ebx], 0        ; EAX

    ; After ret:
    ;
    ;     [esp + 0] = arg
    ;     [esp + 4] = entry
    ;
    sub ebx, 4
    mov [ebx], ecx            ; arg

    sub ebx, 4
    mov [ebx], edx            ; entry

    ; Save resulting ESP.
    mov [eax], ebx

    ret


; ---------------------------------------------------------------------------
; Initial thread bootstrap
;
; On entry:
;
;     ESP -> entry
;            arg
;
; This function converts that into a normal cdecl call:
;
;     entry(arg);
;
; ---------------------------------------------------------------------------

bootstrap_entry:

    mov eax, [esp]            ; eax = entry
    mov edx, [esp + 4]        ; edx = arg

    push edx
    call eax
    add esp, 4

    ; A kernel thread should never return.
    cli

.hang:
    hlt
    jmp .hang