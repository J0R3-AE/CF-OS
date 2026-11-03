#include <kernel/arch/io.h>

// .C port I/O Interrupt control / misc
void sti(void) { __asm__ volatile("sti"); }             // Enable CPU interrupts by setting the IF flag in EFLAGS
void cli(void) { __asm__ volatile("cli"); }            // Disable CPU interrupts by clearing the IF flag in EFLAGS
void wait(void) {out8(0x80, 0); }                               // Wait for an I/O operation to complete by writing to port 0x80