#pragma once
#include "libk/types.h"
#include "shell.h"
#include <stdint.h>

struct shell_ctx; /* forward, if you want context later */

typedef int (*builtin_fn_t)(int argc, char **argv);

typedef struct builtin_cmd {
    const char    *name;
    builtin_fn_t   fn;
    const char    *help;
} builtin_cmd_t;

const builtin_cmd_t *builtin_lookup(const char *name);
void builtin_register_all(void);

/* optional: iterate for `help` */
const builtin_cmd_t *builtin_iter(usize index);
