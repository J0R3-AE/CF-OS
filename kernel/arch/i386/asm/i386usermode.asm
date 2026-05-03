global enter_user_mode
enter_user_mode:
    cli

    mov eax, [esp + 4]   ; eip
    mov ebx, [esp + 8]   ; esp

    ; load user data segments FIRST
    mov ax, 0x23         ; user data selector (ring 3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; build stack for iret
    push 0x23            ; SS
    push ebx             ; ESP
    pushf                ; EFLAGS
    or dword [esp], 0x200 ; IF = 1
    push 0x1B            ; CS (ring 3)
    push eax             ; EIP

    iret