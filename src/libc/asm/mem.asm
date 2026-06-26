; mem.asm
; NASM 32-bit freestanding memory helpers
; cdecl calling convention

BITS 32

SECTION .text

global memset
global memcpy
global memmove
global memcmp
global memchr

; ------------------------------------------------------------
; void *memset(void *dest, int c, usize n)
; ------------------------------------------------------------
memset:
    push ebp
    mov  ebp, esp
    push edi

    mov  edi, [ebp + 8]      ; dest
    mov  eax, [ebp + 12]     ; c
    mov  ecx, [ebp + 16]     ; n
    mov  al, al              ; byte value already in AL
    mov  edx, edi            ; return value = dest

    rep stosb

    mov  eax, edx
    pop  edi
    leave
    ret


; ------------------------------------------------------------
; void *memcpy(void *dest, const void *src, usize n)
; ------------------------------------------------------------
memcpy:
    push ebp
    mov  ebp, esp
    push esi
    push edi

    mov  edi, [ebp + 8]      ; dest
    mov  esi, [ebp + 12]     ; src
    mov  ecx, [ebp + 16]     ; n
    mov  eax, edi            ; return dest

    rep movsb

    pop  edi
    pop  esi
    leave
    ret


; ------------------------------------------------------------
; void *memmove(void *dest, const void *src, usize n)
; ------------------------------------------------------------
memmove:
    push ebp
    mov  ebp, esp
    push esi
    push edi

    mov  edi, [ebp + 8]      ; dest
    mov  esi, [ebp + 12]     ; src
    mov  ecx, [ebp + 16]     ; n
    mov  eax, edi            ; return dest

    cmp  edi, esi
    je   .done               ; same pointer, nothing to do
    jb   .forward            ; dest < src -> copy forward

    ; dest > src -> copy backward
    lea  esi, [esi + ecx - 1]
    lea  edi, [edi + ecx - 1]
    std
    rep movsb
    cld
    jmp  .done

.forward:
    cld
    rep movsb

.done:
    pop  edi
    pop  esi
    leave
    ret


; ------------------------------------------------------------
; int memcmp(const void *s1, const void *s2, usize n)
; ------------------------------------------------------------
memcmp:
    push ebp
    mov  ebp, esp
    push esi
    push edi

    mov  esi, [ebp + 8]      ; s1
    mov  edi, [ebp + 12]     ; s2
    mov  ecx, [ebp + 16]     ; n

    xor  eax, eax
    test ecx, ecx
    jz   .equal

.loop:
    mov  al, [esi]
    mov  dl, [edi]
    cmp  al, dl
    jne  .diff
    inc  esi
    inc  edi
    dec  ecx
    jnz  .loop

.equal:
    xor  eax, eax
    pop  edi
    pop  esi
    leave
    ret

.diff:
    movzx eax, al
    movzx edx, dl
    sub   eax, edx
    pop   edi
    pop   esi
    leave
    ret


; ------------------------------------------------------------
; void *memchr(const void *s, int c, usize n)
; ------------------------------------------------------------
memchr:
    push ebp
    mov  ebp, esp
    push esi

    mov  esi, [ebp + 8]      ; s
    mov  al,  [ebp + 12]     ; c low byte
    mov  ecx, [ebp + 16]     ; n

    test ecx, ecx
    jz   .not_found

.loop:
    cmp  byte [esi], al
    je   .found
    inc  esi
    dec  ecx
    jnz  .loop

.not_found:
    xor  eax, eax
    pop  esi
    leave
    ret

.found:
    mov  eax, esi
    pop  esi
    leave
    ret