global syscall

; int syscall(int num, int a, int b, int c)
syscall:
    push ebx
    push ecx
    push edx

    mov eax, [esp + 16] ; syscall number
    mov ebx, [esp + 20]
    mov ecx, [esp + 24]
    mov edx, [esp + 28]

    int 0x80

    pop edx
    pop ecx
    pop ebx
    ret