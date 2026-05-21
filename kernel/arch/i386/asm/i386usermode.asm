global enter_user_mode
extern klog_user_mode_entry

enter_user_mode:
    cli

    mov eax, [esp + 4]   ; eip
    mov ebx, [esp + 8]   ; esp

    push ebx             ; save esp
    push eax             ; save eip
    push ebx             ; arg2 for klog_user_mode_entry
    push eax             ; arg1 for klog_user_mode_entry
    call klog_user_mode_entry
    add esp, 8           ; remove args
    pop eax              ; restore eip
    pop ebx              ; restore esp

    mov ecx, eax         ; save entry address while we load segments

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
    push ecx             ; EIP

    iret