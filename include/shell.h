#pragma once

#include "libk/types.h"

extern char cwd[128];

void shell_main(void *arg);

/* exec function type for VFS executable nodes */
//typedef void (*exec_fn_t)(const char *args);

/* built-in commands */
void sh_cmd_ls(const char *args);
void sh_cmd_cat(const char *args);
void sh_cmd_echo(const char *args);

static void sh_add_history(const char *line);

/* resolve relative path against cwd into out (absolute) */
static void sh_resolve_path(const char *input, char *out, usize out_sz);