#include "kernel/arch/io.h"

/* MMIO Read */
u64 mmin64(void *p) { return *(volatile u64 *)p; }                 // 64bit read

/* MMIO Write*/
void mmout64(void *p, u64 data) { *(volatile u64 *)p = data; }     // 64bit write