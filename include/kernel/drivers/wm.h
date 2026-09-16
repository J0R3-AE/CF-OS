#ifndef KERNEL_DRIVERS_WM_H
#define KERNEL_DRIVERS_WM_H

#include <stdint.h>

/* Initialize the simple framebuffer-based window manager. */
void wm_init(void);

/* Toggle the debug overlay on/off. */
void wm_toggle(void);

/* Return non-zero if overlay is visible */
int wm_is_visible(void);

#endif
