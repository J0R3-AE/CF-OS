#include "arch/io.h"

/* IO Read */
u8 io_Read8(u16 port) {u8 data ;__asm__ volatile("inb %w1, %b0" : "=a"(data) : "Nd"(port)); return data;}       // 8bit read
u16 io_Read16(u16 port) {u16 data; __asm__ volatile("inw %w1, %w0" : "=a"(data) : "Nd"(port)); return data;}    // 16bit read
u32 io_Read32(u16 port) {u32 data; __asm__ volatile("inl %w1, %0" : "=a"(data) : "Nd"(port)); return data;}     // 32bit read

/* IO Write */
void io_Write8(u16 port, u8 data) {__asm__ volatile("outb %b0, %w1" : : "a"(data), "Nd"(port));}    // 8bit write
void io_Write16(u16 port, u16 data) {__asm__ volatile("outw %w0, %w1" : : "a"(data), "Nd"(port));}  // 16bit write
void io_Write32(u16 port, u32 data) {__asm__ volatile("outl %0, %w1" : : "a"(data), "Nd"(port));}   // 32bit write

/* MMIO Read */
u8 mmio_Read8(void *p) { return *(volatile u8 *)p; }                    // 8bit read
u16 mmio_Read16(void *p) { return *(volatile u16 *)p; }                 // 16bit read
u32 mmio_Read32(void *p) { return *(volatile u32 *)p; }                 // 32bit read

/* MMIO Write*/
void mmio_Write8(void *p, u8 data) { *(volatile u8 *)p = data; }        // 8bit write
void mmio_Write16(void *p, u16 data) { *(volatile u16 *)p = data; }     // 16bit write
void mmio_Write32(void *p, u32 data) { *(volatile u32 *)p = data; }     // 32bit write

// .C port I/O Interrupt control / misc
void io_enableinterrupts(void) { __asm__ volatile("sti"); }             // Enable CPU interrupts by setting the IF flag in EFLAGS
void io_disableinterrupts(void) { __asm__ volatile("cli"); }            // Disable CPU interrupts by clearing the IF flag in EFLAGS
void io_halt(void) { __asm__ volatile("hlt"); }                         // Halt the CPU until the next external interrupt is received
void io_wait(void) {io_Write8(0x80, 0); }                               // Wait for an I/O operation to complete by writing to port 0x80

