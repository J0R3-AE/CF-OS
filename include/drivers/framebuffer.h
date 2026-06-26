#pragma once
#include <stdint.h>
#include <stddef.h>

void framebuffer_init(uint32_t addr, uint32_t w, uint32_t h,
                      uint32_t pitch, uint32_t bpp);

void framebuffer_putpixel(int x, int y, uint32_t color);
void framebuffer_fill(uint32_t color);

uint32_t framebuffer_width(void);
uint32_t framebuffer_height(void);
uint32_t framebuffer_pitch(void);
uint32_t framebuffer_bpp(void);