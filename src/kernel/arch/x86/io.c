#include "kernel/arch/io.h"

/* IO Read */
u8 in8(u16 port) {u8 data ;__asm__ volatile("inb %w1, %b0" : "=a"(data) : "Nd"(port)); return data;}       // 8bit read
u16 in16(u16 port) {u16 data; __asm__ volatile("inw %w1, %w0" : "=a"(data) : "Nd"(port)); return data;}    // 16bit read
u32 in32(u16 port) {u32 data; __asm__ volatile("inl %w1, %0" : "=a"(data) : "Nd"(port)); return data;}     // 32bit read

/* IO Write */
void out8(u16 port, u8 data) {__asm__ volatile("outb %b0, %w1" : : "a"(data), "Nd"(port));}    // 8bit write
void out16(u16 port, u16 data) {__asm__ volatile("outw %w0, %w1" : : "a"(data), "Nd"(port));}  // 16bit write
void out32(u16 port, u32 data) {__asm__ volatile("outl %0, %w1" : : "a"(data), "Nd"(port));}   // 32bit write

/* MMIO Read */
u8 mmin8(void *p) { return *(volatile u8 *)p; }                    // 8bit read
u16 mmin16(void *p) { return *(volatile u16 *)p; }                 // 16bit read
u32 mmin32(void *p) { return *(volatile u32 *)p; }                 // 32bit read

/* MMIO Write*/
void mmout8(void *p, u8 data) { *(volatile u8 *)p = data; }        // 8bit write
void mmout16(void *p, u16 data) { *(volatile u16 *)p = data; }     // 16bit write
void mmout32(void *p, u32 data) { *(volatile u32 *)p = data; }     // 32bit write