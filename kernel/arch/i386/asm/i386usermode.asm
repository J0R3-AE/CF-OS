global enter_user_mode

section .text

enter_user_mode:
    ; cdecl:
    ; [esp+4] = entry
    ; [esp+8] = user_esp

    mov eax, [esp + 4]     ; entry
    mov edx, [esp + 8]     ; user stack

    ; -----------------------------
    ; Load user data segments
    ; -----------------------------
    mov ax, 0x23           ; user data selector (RPL 3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; -----------------------------
    ; Build iret frame
    ; -----------------------------

    push dword 0x23        ; SS (user stack segment)
    push edx               ; ESP (user stack pointer)

    pushfd                 ; EFLAGS
    pop ecx
    or ecx, 0x200          ; ensure interrupts enabled
    push ecx

    mov cx, 0x1B           ; CS (user code selector)
    push ecx

    push eax               ; EIP (entry point)

    iretd