#ifndef INPUT_H
#define INPUT_H

#include "drivers/keyboard.h"
#include "drivers/tty.h"
#include "input.h"

int readline(const char *prompt, char *buf, int max);

#endif