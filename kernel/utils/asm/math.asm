BITS 32

global abs_i32
global min_i32
global max_i32
global clamp_i32

global min_u32
global max_u32
global clamp_u32

global align_up
global align_down
global round_up_u32
global round_down_u32

global ceil_div_u32
global floor_div_u32

global pow_u32
global mod_pow_u32
global sqrt_u32

global gcd_u32
global lcm_u32

global is_prime_u32
global next_prime_u32
global factor_smallest_u32

global log2_floor_u32
global log2_ceil_u32
global popcount_u32
global parity_u32

global factorial_u32
global fibonacci_u32

section .text

; int abs_i32(int x)
abs_i32:
    mov eax, [esp + 4]
    test eax, eax
    jns .done
    neg eax
.done:
    ret

; int min_i32(int a, int b)
min_i32:
    mov eax, [esp + 4]
    mov edx, [esp + 8]
    cmp eax, edx
    jle .done
    mov eax, edx
.done:
    ret

; int max_i32(int a, int b)
max_i32:
    mov eax, [esp + 4]
    mov edx, [esp + 8]
    cmp eax, edx
    jge .done
    mov eax, edx
.done:
    ret

; int clamp_i32(int v, int lo, int hi)
clamp_i32:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    mov edx, [esp + 12]

    cmp eax, ecx
    jge .check_hi
    mov eax, ecx
    ret

.check_hi:
    cmp eax, edx
    jle .done
    mov eax, edx
.done:
    ret

; u32 min_u32(u32 a, u32 b)
min_u32:
    mov eax, [esp + 4]
    mov edx, [esp + 8]
    cmp eax, edx
    jbe .done
    mov eax, edx
.done:
    ret

; u32 max_u32(u32 a, u32 b)
max_u32:
    mov eax, [esp + 4]
    mov edx, [esp + 8]
    cmp eax, edx
    jae .done
    mov eax, edx
.done:
    ret

; u32 clamp_u32(u32 v, u32 lo, u32 hi)
clamp_u32:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    mov edx, [esp + 12]

    cmp eax, ecx
    jae .check_hi
    mov eax, ecx
    ret

.check_hi:
    cmp eax, edx
    jbe .done
    mov eax, edx
.done:
    ret

; u32 align_up(u32 value, u32 align)
align_up:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    dec ecx
    add eax, ecx
    not ecx
    and eax, ecx
    ret

; u32 align_down(u32 value, u32 align)
align_down:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    dec ecx
    not ecx
    and eax, ecx
    ret

; u32 round_up_u32(u32 value, u32 multiple)
round_up_u32:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    test ecx, ecx
    jz .done
    dec ecx
    add eax, ecx
    not ecx
    and eax, ecx
.done:
    ret

; u32 round_down_u32(u32 value, u32 multiple)
round_down_u32:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    test ecx, ecx
    jz .done
    dec ecx
    not ecx
    and eax, ecx
.done:
    ret

; u32 ceil_div_u32(u32 a, u32 b)
ceil_div_u32:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    test ecx, ecx
    jz .done
    dec eax
    xor edx, edx
    div ecx
    inc eax
.done:
    ret

; u32 floor_div_u32(u32 a, u32 b)
floor_div_u32:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    test ecx, ecx
    jz .done
    xor edx, edx
    div ecx
.done:
    ret

; u32 pow_u32(u32 base, u32 exp)
pow_u32:
    mov eax, 1
    mov edx, [esp + 4]
    mov ecx, [esp + 8]
.loop:
    test ecx, ecx
    jz .done
    imul eax, edx
    dec ecx
    jmp .loop
.done:
    ret

; u32 mod_pow_u32(u32 base, u32 exp, u32 mod)
; naive modular exponentiation
mod_pow_u32:
    mov eax, 1
    mov edx, [esp + 4]     ; base
    mov ecx, [esp + 8]     ; exp
    mov esi, [esp + 12]    ; mod
    test esi, esi
    jz .done

.loop:
    test ecx, ecx
    jz .done

    test cl, 1
    jz .skip_mul
    imul eax, edx
    xor edx, edx
    div esi
    mov eax, edx
    mov edx, [esp + 4]
.skip_mul:
    imul edx, edx
    xor edx, edx
    div esi
    mov edx, edx
    shr ecx, 1
    jmp .loop

.done:
    ret

; u32 sqrt_u32(u32 n)
; integer sqrt by linear increment (simple, safe, slow)
sqrt_u32:
    mov ecx, [esp + 4]
    xor eax, eax
.loop:
    mov edx, eax
    imul edx, eax
    cmp edx, ecx
    ja .finish
    inc eax
    jmp .loop
.finish:
    dec eax
    ret

; u32 gcd_u32(u32 a, u32 b)
gcd_u32:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    test ecx, ecx
    jz .done
.loop:
    xor edx, edx
    div ecx
    mov eax, ecx
    mov ecx, edx
    test ecx, ecx
    jnz .loop
.done:
    ret

; u32 lcm_u32(u32 a, u32 b)
lcm_u32:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    test eax, eax
    jz .done
    test ecx, ecx
    jz .done

    push ecx
    push eax
    call gcd_u32
    add esp, 8

    mov ecx, eax
    mov eax, [esp + 4]
    xor edx, edx
    div ecx
    imul eax, [esp + 8]
.done:
    ret

; int is_prime_u32(u32 n)
is_prime_u32:
    mov eax, [esp + 4]
    cmp eax, 2
    jb .notprime
    je .prime
    test eax, 1
    jz .notprime

    mov ecx, 3
.loop:
    mov edx, ecx
    imul edx, ecx
    cmp edx, eax
    ja .prime

    xor edx, edx
    div ecx
    test edx, edx
    jz .notprime

    add ecx, 2
    mov eax, [esp + 4]
    jmp .loop

.prime:
    mov eax, 1
    ret

.notprime:
    xor eax, eax
    ret

; u32 next_prime_u32(u32 n)
next_prime_u32:
    mov eax, [esp + 4]
    cmp eax, 2
    jb .ret2
    cmp eax, 2
    je .ret2
    test eax, 1
    jnz .check
    inc eax
.check:
.loop:
    push eax
    call is_prime_u32
    add esp, 4
    test eax, eax
    jnz .done
    add eax, 2
    jmp .loop
.ret2:
    mov eax, 2
.done:
    ret

; u32 factor_smallest_u32(u32 n)
; returns smallest factor > 1, or n if prime/1/0
factor_smallest_u32:
    mov eax, [esp + 4]
    cmp eax, 2
    jb .done
    test eax, 1
    jz .even

    mov ecx, 3
.loop:
    mov edx, ecx
    imul edx, ecx
    cmp edx, eax
    ja .done

    push eax
    push ecx
    call ceil_div_u32
    add esp, 8

    ; trial division: n % ecx
    mov eax, [esp + 4]
    xor edx, edx
    div ecx
    test edx, edx
    jz .found

    add ecx, 2
    mov eax, [esp + 4]
    jmp .loop

.even:
    mov eax, 2
    ret

.found:
    mov eax, ecx
.done:
    ret

; u32 log2_floor_u32(u32 n)
log2_floor_u32:
    mov eax, [esp + 4]
    xor ecx, ecx
    test eax, eax
    jz .done
.loop:
    shr eax, 1
    jz .done
    inc ecx
    jmp .loop
.done:
    mov eax, ecx
    ret

; u32 log2_ceil_u32(u32 n)
log2_ceil_u32:
    mov eax, [esp + 4]
    test eax, eax
    jz .done
    dec eax
    push eax
    call log2_floor_u32
    add esp, 4
    inc eax
.done:
    ret

; u32 popcount_u32(u32 n)
popcount_u32:
    mov eax, [esp + 4]
    xor ecx, ecx
.loop:
    test eax, eax
    jz .done
    mov edx, eax
    and edx, 1
    add ecx, edx
    shr eax, 1
    jmp .loop
.done:
    mov eax, ecx
    ret

; int parity_u32(u32 n)
; returns 0 for even parity, 1 for odd parity
parity_u32:
    push dword [esp + 4]
    call popcount_u32
    add esp, 4
    and eax, 1
    ret

; u32 factorial_u32(u32 n)
factorial_u32:
    mov eax, 1
    mov ecx, [esp + 4]
    cmp ecx, 1
    jbe .done
.loop:
    imul eax, ecx
    dec ecx
    cmp ecx, 1
    ja .loop
.done:
    ret

; u32 fibonacci_u32(u32 n)
fibonacci_u32:
    mov ecx, [esp + 4]
    cmp ecx, 0
    je .zero
    cmp ecx, 1
    je .one

    xor eax, eax    ; a = 0
    mov edx, 1      ; b = 1
.loop:
    lea ebx, [eax + edx]
    mov eax, edx
    mov edx, ebx
    dec ecx
    cmp ecx, 1
    ja .loop
    mov eax, edx
    ret

.zero:
    xor eax, eax
    ret

.one:
    mov eax, 1
    ret