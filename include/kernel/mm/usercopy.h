#pragma once

#include "libc/types.h"

int copy_from_user(void *kernel_dst, const void *user_src, usize length);
int copy_to_user(void *user_dst, const void *kernel_src, usize length);
int copy_user_string(char *kernel_dst, usize capacity, const char *user_src);

