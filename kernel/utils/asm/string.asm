; string.asm
; NASM 32-bit freestanding string helpers
; cdecl calling convention
;
; Externals needed:
;   malloc
;   memcpy

BITS 32

SECTION .text

global strlen
global strcpy
global strncpy
global strchr
global strrchr
global strcmp
global strncmp
global strcasecmp
global strstr
global strcat
global strncat
global strspn
global strcspn
global strdup
global strndup
global strtok

extern malloc
extern memcpy

; ------------------------------------------------------------
; usize strlen(const char *s)
; ------------------------------------------------------------
strlen:
    push ebp
    mov  ebp, esp

    mov  eax, [ebp + 8]      ; s
    mov  edx, eax            ; p = s

.len_loop:
    cmp  byte [edx], 0
    je   .len_done
    inc  edx
    jmp  .len_loop

.len_done:
    sub  edx, eax            ; p - s
    mov  eax, edx

    leave
    ret


; ------------------------------------------------------------
; char *strcpy(char *dest, const char *src)
; ------------------------------------------------------------
strcpy:
    push ebp
    mov  ebp, esp
    push esi
    push edi

    mov  edi, [ebp + 8]      ; dest
    mov  esi, [ebp + 12]     ; src
    mov  eax, edi            ; return dest

.copy_loop:
    mov  dl, [esi]
    mov  [edi], dl
    inc  esi
    inc  edi
    test dl, dl
    jne  .copy_loop

    pop  edi
    pop  esi
    leave
    ret


; ------------------------------------------------------------
; char *strncpy(char *dest, const char *src, usize n)
; ------------------------------------------------------------
strncpy:
    push ebp
    mov  ebp, esp
    push esi
    push edi

    mov  edi, [ebp + 8]      ; dest
    mov  esi, [ebp + 12]     ; src
    mov  ecx, [ebp + 16]     ; n
    mov  eax, edi            ; return dest

    test ecx, ecx
    jz   .done

    xor  edx, edx            ; i = 0

.copy_part:
    cmp  edx, ecx
    jae  .pad_zero
    mov  bl, [esi + edx]
    test bl, bl
    jz   .pad_zero
    mov  [edi + edx], bl
    inc  edx
    jmp  .copy_part

.pad_zero:
    cmp  edx, ecx
    jae  .done
    mov  byte [edi + edx], 0
    inc  edx
    jmp  .pad_zero

.done:
    pop  edi
    pop  esi
    leave
    ret


; ------------------------------------------------------------
; char *strchr(const char *s, int c)
; Standard behavior: returns pointer to '\0' if c == 0
; ------------------------------------------------------------
strchr:
    push ebp
    mov  ebp, esp

    mov  esi, [ebp + 8]      ; s
    mov  dl,  [ebp + 12]     ; c low byte

.loop:
    mov  al, [esi]
    cmp  al, dl
    je   .found
    cmp  al, 0
    je   .not_found
    inc  esi
    jmp  .loop

.found:
    mov  eax, esi
    leave
    ret

.not_found:
    xor  eax, eax
    leave
    ret


; ------------------------------------------------------------
; char *strrchr(const char *s, int c)
; Standard behavior: if c == 0, returns pointer to terminator
; ------------------------------------------------------------
strrchr:
    push ebp
    mov  ebp, esp

    mov  esi, [ebp + 8]      ; s
    mov  dl,  [ebp + 12]     ; c low byte
    xor  eax, eax            ; last = NULL

.loop:
    mov  cl, [esi]
    cmp  cl, dl
    jne  .check_end
    mov  eax, esi            ; last = current match
    cmp  cl, 0
    je   .done               ; if searching for '\0', this is the terminator

.check_end:
    cmp  cl, 0
    je   .done
    inc  esi
    jmp  .loop

.done:
    leave
    ret


; ------------------------------------------------------------
; int strcmp(const char *a, const char *b)
; ------------------------------------------------------------
strcmp:
    push ebp
    mov  ebp, esp

    mov  esi, [ebp + 8]
    mov  edi, [ebp + 12]

.loop:
    mov  al, [esi]
    mov  dl, [edi]
    cmp  al, dl
    jne  .diff
    test al, al
    je   .equal
    inc  esi
    inc  edi
    jmp  .loop

.diff:
    movzx eax, al
    movzx edx, dl
    sub   eax, edx
    leave
    ret

.equal:
    xor  eax, eax
    leave
    ret


; ------------------------------------------------------------
; int strncmp(const char *a, const char *b, usize n)
; ------------------------------------------------------------
strncmp:
    push ebp
    mov  ebp, esp

    mov  esi, [ebp + 8]
    mov  edi, [ebp + 12]
    mov  ecx, [ebp + 16]

    test ecx, ecx
    jz   .equal

    xor  edx, edx            ; i = 0

.loop:
    cmp  edx, ecx
    jae  .equal

    mov  al, [esi + edx]
    mov  bl, [edi + edx]
    cmp  al, bl
    jne  .diff
    test al, al
    je   .equal
    inc  edx
    jmp  .loop

.diff:
    movzx eax, al
    movzx edx, bl
    sub   eax, edx
    leave
    ret

.equal:
    xor  eax, eax
    leave
    ret


; ------------------------------------------------------------
; int strcasecmp(const char *a, const char *b)
; ASCII-only case folding
; ------------------------------------------------------------
strcasecmp:
    push ebp
    mov  ebp, esp
    push esi
    push edi

    mov  esi, [ebp + 8]
    mov  edi, [ebp + 12]

.loop:
    mov  al, [esi]
    mov  dl, [edi]

    test al, al
    jz   .finish
    test dl, dl
    jz   .finish

    ; tolower_ascii(al)
    cmp  al, 'A'
    jb   .a_done
    cmp  al, 'Z'
    ja   .a_done
    add  al, 32
.a_done:

    ; tolower_ascii(dl)
    cmp  dl, 'A'
    jb   .b_done
    cmp  dl, 'Z'
    ja   .b_done
    add  dl, 32
.b_done:

    cmp  al, dl
    jne  .diff
    inc  esi
    inc  edi
    jmp  .loop

.finish:
    ; compare final characters after ASCII lowering
    mov  al, [esi]
    mov  dl, [edi]

    cmp  al, 'A'
    jb   .a_done2
    cmp  al, 'Z'
    ja   .a_done2
    add  al, 32
.a_done2:
    cmp  dl, 'A'
    jb   .b_done2
    cmp  dl, 'Z'
    ja   .b_done2
    add  dl, 32
.b_done2:
    movzx eax, al
    movzx edx, dl
    sub   eax, edx
    pop   edi
    pop   esi
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
; char *strstr(const char *haystack, const char *needle)
; ------------------------------------------------------------
strstr:
    push ebp
    mov  ebp, esp
    push esi
    push edi
    push ebx

    mov  esi, [ebp + 8]      ; haystack
    mov  edi, [ebp + 12]     ; needle

    cmp  byte [edi], 0
    je   .found_here         ; empty needle -> haystack

.outer:
    cmp  byte [esi], 0
    je   .not_found

    mov  ebx, esi            ; h = haystack position
    mov  edx, edi            ; n = needle

.inner:
    mov  al, [ebx]
    cmp  al, 0
    je   .check_needle_end
    cmp  al, [edx]
    jne  .next_haystack
    inc  ebx
    inc  edx
    jmp  .inner

.check_needle_end:
    cmp  byte [edx], 0
    je   .found_here
    jmp  .next_haystack

.next_haystack:
    inc  esi
    jmp  .outer

.found_here:
    mov  eax, esi
    pop  ebx
    pop  edi
    pop  esi
    leave
    ret

.not_found:
    xor  eax, eax
    pop  ebx
    pop  edi
    pop  esi
    leave
    ret


; ------------------------------------------------------------
; char *strcat(char *dest, const char *src)
; ------------------------------------------------------------
strcat:
    push ebp
    mov  ebp, esp
    push esi
    push edi

    mov  edi, [ebp + 8]      ; dest
    mov  esi, [ebp + 12]     ; src
    mov  eax, edi            ; return dest

.find_end:
    cmp  byte [edi], 0
    je   .copy
    inc  edi
    jmp  .find_end

.copy:
    mov  dl, [esi]
    mov  [edi], dl
    inc  esi
    inc  edi
    test dl, dl
    jne  .copy

    pop  edi
    pop  esi
    leave
    ret


; ------------------------------------------------------------
; char *strncat(char *dest, const char *src, usize n)
; ------------------------------------------------------------
strncat:
    push ebp
    mov  ebp, esp
    push esi
    push edi

    mov  edi, [ebp + 8]      ; dest
    mov  esi, [ebp + 12]     ; src
    mov  ecx, [ebp + 16]     ; n
    mov  eax, edi            ; return dest

.find_end:
    cmp  byte [edi], 0
    je   .copy
    inc  edi
    jmp  .find_end

.copy:
    xor  edx, edx            ; i = 0

.copy_loop:
    cmp  edx, ecx
    jae  .terminate
    mov  bl, [esi + edx]
    test bl, bl
    jz   .terminate
    mov  [edi + edx], bl
    inc  edx
    jmp  .copy_loop

.terminate:
    mov  byte [edi + edx], 0

    pop  edi
    pop  esi
    leave
    ret


; ------------------------------------------------------------
; usize strspn(const char *s, const char *accept)
; ------------------------------------------------------------
strspn:
    push ebp
    mov  ebp, esp
    push esi
    push edi
    push ebx

    mov  esi, [ebp + 8]      ; s
    mov  edi, [ebp + 12]     ; accept
    xor  eax, eax            ; count = 0

.outer:
    mov  bl, [esi]
    cmp  bl, 0
    je   .done

    mov  edx, edi            ; scan accept
    xor  ecx, ecx            ; matched = 0

.inner:
    mov  dl, [edx]
    cmp  dl, 0
    je   .not_matched
    cmp  dl, bl
    je   .matched
    inc  edx
    jmp  .inner

.matched:
    inc  eax
    inc  esi
    jmp  .outer

.not_matched:
    jmp  .done

.done:
    pop  ebx
    pop  edi
    pop  esi
    leave
    ret


; ------------------------------------------------------------
; usize strcspn(const char *s, const char *reject)
; ------------------------------------------------------------
strcspn:
    push ebp
    mov  ebp, esp
    push esi
    push edi
    push ebx

    mov  esi, [ebp + 8]      ; s
    mov  edi, [ebp + 12]     ; reject
    xor  eax, eax            ; count = 0

.outer:
    mov  bl, [esi]
    cmp  bl, 0
    je   .done

    mov  edx, edi            ; scan reject

.inner:
    mov  dl, [edx]
    cmp  dl, 0
    je   .not_found_in_reject
    cmp  dl, bl
    je   .done
    inc  edx
    jmp  .inner

.not_found_in_reject:
    inc  eax
    inc  esi
    jmp  .outer

.done:
    pop  ebx
    pop  edi
    pop  esi
    leave
    ret


; ------------------------------------------------------------
; char *strdup(const char *src)
; malloc + memcpy
; ------------------------------------------------------------
strdup:
    push ebp
    mov  ebp, esp
    sub  esp, 4              ; local: len

    mov  eax, [ebp + 8]      ; src
    push eax
    call strlen
    add  esp, 4

    inc  eax                 ; strlen + 1
    mov  [ebp - 4], eax

    push eax
    call malloc
    add  esp, 4

    test eax, eax
    jz   .null

    ; memcpy(dst, src, len)
    push dword [ebp - 4]
    push dword [ebp + 8]
    push eax
    call memcpy
    add  esp, 12

    leave
    ret

.null:
    xor  eax, eax
    leave
    ret


; ------------------------------------------------------------
; char *strndup(const char *src, usize maxlen)
; malloc + memcpy
; ------------------------------------------------------------
strndup:
    push ebp
    mov  ebp, esp
    sub  esp, 4              ; local: n

    mov  eax, [ebp + 8]      ; src
    test eax, eax
    jz   .null

    mov  ecx, [ebp + 16]     ; maxlen
    xor  edx, edx            ; n = 0

.len_loop:
    cmp  edx, ecx
    jae  .alloc
    cmp  byte [eax + edx], 0
    je   .alloc
    inc  edx
    jmp  .len_loop

.alloc:
    mov  [ebp - 4], edx
    mov  eax, edx
    inc  eax                 ; n + 1
    push eax
    call malloc
    add  esp, 4

    test eax, eax
    jz   .null

    ; memcpy(dst, src, n)
    push dword [ebp - 4]
    push dword [ebp + 8]
    push eax
    call memcpy
    add  esp, 12

    ; dst[n] = '\0'
    mov  edx, [ebp - 4]
    mov  byte [eax + edx], 0

    leave
    ret

.null:
    xor  eax, eax
    leave
    ret


; ------------------------------------------------------------
; char *strtok(char *s, const char *delim)
; Stateful tokenizer with internal static pointer
; ------------------------------------------------------------
strtok:
    push ebp
    mov  ebp, esp
    push esi
    push edi
    push ebx

    mov  eax, [ebp + 8]      ; s
    test eax, eax
    jz   .use_state

    mov  [strtok_state], eax ; new string starts tokenization

.use_state:
    mov  esi, [strtok_state]
    test esi, esi
    jz   .null

.skip_delim:
    mov  bl, [esi]
    cmp  bl, 0
    je   .end_of_string

    mov  edi, [ebp + 12]     ; delim
    mov  edx, edi

.check_delim:
    mov  al, [edx]
    cmp  al, 0
    je   .start_token
    cmp  al, bl
    je   .advance_and_retry
    inc  edx
    jmp  .check_delim

.advance_and_retry:
    inc  esi
    jmp  .skip_delim

.start_token:
    mov  eax, esi            ; token start

.scan_token:
    mov  bl, [esi]
    cmp  bl, 0
    je   .finish_last
    mov  edi, [ebp + 12]
    mov  edx, edi

.scan_delim:
    mov  cl, [edx]
    cmp  cl, 0
    je   .not_delim
    cmp  cl, bl
    je   .split_here
    inc  edx
    jmp  .scan_delim

.not_delim:
    inc  esi
    jmp  .scan_token

.split_here:
    mov  byte [esi], 0
    inc  esi
    mov  [strtok_state], esi
    jmp  .return_token

.finish_last:
    mov  dword [strtok_state], 0
    jmp  .return_token

.end_of_string:
    mov  dword [strtok_state], 0
    jmp  .null

.return_token:
    pop  ebx
    pop  edi
    pop  esi
    leave
    ret

.null:
    xor  eax, eax
    pop  ebx
    pop  edi
    pop  esi
    leave
    ret


SECTION .bss
strtok_state: resd 1