[BITS 32]

GLOBAL i386SPINLOCK_lock
GLOBAL i386SPINLOCK_unlock
GLOBAL i386SPINLOCK_trylock
GLOBAL i386SPINLOCK_force_lock
GLOBAL i386SPINLOCK_force_unlock
GLOBAL i386SPINLOCK_destroy
GLOBAL i386SPINLOCK_wait
GlOBAL i386SPINLOCK_state

; Spinlock implementation for i386 architecture
; The lock is represented as a 32-bit integer where 0 means unlocked and 1 means locked.

; Function: i386SPINLOCK_lock
; Description: Acquires the spinlock. If the lock is already held, it will spin until it can acquire it.
; Parameters:
;   - [esp + 4]: Pointer to the spinlock variable (32-bit integer)
; Returns: None
i386SPINLOCK_lock:
    mov eax, 1
    xchg [esp + 4], eax
    test eax, eax
    jz .locked
.spin:
    pause
    mov eax, 1
    xchg [esp + 4], eax
    test eax, eax
    jnz .spin
.locked:
    ret

; Function: i386SPINLOCK_unlock
; Description: Releases the spinlock by setting it back to 0.
; Parameters:
;   - [esp + 4]: Pointer to the spinlock variable (32-bit integer)
; Returns: None
i386SPINLOCK_unlock:
    mov eax, 0
    xchg [esp + 4], eax
    test eax, eax
    jz .unlocked
.spin:
    pause
    mov eax, 0
    xchg [esp + 4], eax
    test eax, eax
    jnz .spin
.unlocked:
    ret

; Function: i386SPINLOCK_trylock
; Description: Attempts to acquire the spinlock without blocking. Returns 1 if the lock was successfully acquired, or 0 if it was already held.
; Parameters:
;   - [esp + 4]: Pointer to the spinlock variable (32-bit integer)
; Returns: 1 if lock acquired, 0 if lock is already held
i386SPINLOCK_trylock:
    mov eax, 1
    xchg [esp + 4], eax
    test eax, eax
    jz .locked
    xor eax, eax
.locked:
    ret 

; Function: i386SPINLOCK_force_lock
; Description: Forcefully acquires the spinlock without checking its current state. This should be used with caution as it can lead to deadlocks.
; Parameters:
;   - [esp + 4]: Pointer to the spinlock variable (32-bit integer)
; Returns: None
i386SPINLOCK_force_lock:
    mov dword [esp + 4], 1
    ret

; Function: i386SPINLOCK_force_unlock
; Description: Forcefully releases the spinlock without checking its current state. This should be used with caution as it can lead to inconsistent states.
; Parameters:
;   - [esp + 4]: Pointer to the spinlock variable (32-bit integer)
; Returns: None
i386SPINLOCK_force_unlock:
    mov dword [esp + 4], 0
    ret

; Function: i386SPINLOCK_destroy
; Description: Destroys the spinlock by setting it to an invalid state (e.g., -1). This can be used to indicate that the lock is no longer usable.
; Parameters:
;   - [esp + 4]: Pointer to the spinlock variable (32-bit integer)
; Returns: None
i386SPINLOCK_destroy:
    mov dword [esp + 4], -1
    ret

; Function: i386SPINLOCK_wait
; Description: Waits for the spinlock to become available. This is a blocking call that will spin until the lock can be acquired.
; Parameters:
;   - [esp + 4]: Pointer to the spinlock variable (32-bit integer)
; Returns: None
i386SPINLOCK_wait:
    mov eax, 1
    xchg [esp + 4], eax
    test eax, eax
    jz .locked  
.wait_loop:
    mov eax, 1
    xchg [esp + 4], eax
    test eax, eax
    jz .locked
    pause
    jmp .wait_loop
.locked:
    ret

; Function: i386SPINLOCK_state
; Description: Checks the current state of the spinlock. Returns 1 if the lock is currently held, or 0 if it is unlocked.
; Parameters:
;   - [esp + 4]: Pointer to the spinlock variable (32-bit integer)
; Returns: 1 if lock is held, 0 if lock is unlocked
i386SPINLOCK_state:
    mov eax, [esp + 4]
    test eax, eax
    jz .unlocked
    mov eax, 1
    ret
.unlocked:
    xor eax, eax
    ret
