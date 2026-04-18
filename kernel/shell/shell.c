#include "shell.h"
#include "drivers/tty.h"
#include "drivers/input.h"
#include "libk/string.h"
#include "libk/printf.h"
#include "fs/vfs.h"

/* -------------------------
   Shell state
   ------------------------- */

char cwd[128] = "/";

#define SHELL_HISTORY_MAX 32
#define SHELL_LINE_MAX    128

static char sh_history[SHELL_HISTORY_MAX][SHELL_LINE_MAX];
static int  sh_history_count = 0;

static int shell_running = 1;

extern int builtin_cd(int argc, char **argv);
extern int builtin_pwd(int argc, char **argv);
extern int builtin_echo(int argc, char **argv);
extern int builtin_clear(int argc, char **argv);
extern int builtin_exit(int argc, char **argv);
extern int builtin_help(int argc, char **argv);
extern int builtin_history(int argc, char **argv);
extern int builtin_ls(int argc, char **argv);
extern int builtin_cat(int argc, char **argv);


/* -------------------------
   Helpers
   ------------------------- */

static void sh_add_history(const char *line)
{
    if (!line || !*line)
        return;

    if (sh_history_count < SHELL_HISTORY_MAX) {
        strcpy(sh_history[sh_history_count++], line);
    } else {
        for (int i = 1; i < SHELL_HISTORY_MAX; i++)
            strcpy(sh_history[i - 1], sh_history[i]);
        strcpy(sh_history[SHELL_HISTORY_MAX - 1], line);
    }
}

/* resolve relative path against cwd into out (absolute) */
static void sh_resolve_path(const char *input, char *out, usize out_sz)
{
    if (!input || !*input) {
        /* no arg: use cwd */
        strncpy(out, cwd, out_sz - 1);
        out[out_sz - 1] = '\0';
        return;
    }

    if (input[0] == '/') {
        strncpy(out, input, out_sz - 1);
        out[out_sz - 1] = '\0';
        return;
    }

    /* cwd + "/" + input */
    out[0] = '\0';
    strncpy(out, cwd, out_sz - 1);
    out[out_sz - 1] = '\0';

    if (out[strlen(out) - 1] != '/') {
        if (strlen(out) + 1 < out_sz - 1)
            strcat(out, "/");
    }

    if (strlen(out) + strlen(input) < out_sz - 1)
        strcat(out, input);
}

/* -------------------------
   Built-in registry
   ------------------------- */

typedef struct {
    const char *name;
    exec_fn_t   fn;
} sh_builtin_t;

static sh_builtin_t sh_builtins[] = {
    { "cd",      builtin_cd      },
    { "exit",    builtin_exit    },
    { "pwd",     builtin_pwd     },
    { "echo",    builtin_echo    },
    { "clear",   builtin_clear   },
    { "history", builtin_history },
    { "ls",      builtin_ls      }, /* also available via /bin/ls if you want */
    { "cat",     builtin_cat     },
    { "help",    NULL           }, /* handled specially */
    { NULL,      NULL           }
};

static exec_fn_t sh_find_builtin(const char *name)
{
    for (int i = 0; sh_builtins[i].name; i++) {
        if (strcmp(sh_builtins[i].name, name) == 0)
            return sh_builtins[i].fn;
    }
    return NULL;
}

static void sh_print_help(void)
{
    printf("Built-in commands:\n");
    for (int i = 0; sh_builtins[i].name; i++) {
        printf("  %s\n", sh_builtins[i].name);
    }
}

/* -------------------------
   Command dispatcher
   ------------------------- */
static void sh_dispatch_line(char *line)
{
    while (*line == ' ') line++;
    if (*line == '\0')
        return;

    char *cmd = line;
    char *args = line;

    while (*args && *args != ' ')
        args++;
    if (*args) {
        *args++ = '\0';
        while (*args == ' ') args++;
    }

    if (strcmp(cmd, "help") == 0) {
        sh_print_help();
        return;
    }

    /* built-in? */
    exec_fn_t fn = sh_find_builtin(cmd);
    if (fn) {
        fn(args);
        return;
    }

    /* external command in /bin */
    char path[128];
    path[0] = '\0';
    strcpy(path, "/bin/");
    strcat(path, cmd);

    struct vnode *node;
    if (vfs_lookup(path, &node) == 0) {

        // kernel exec nodes
        if (node->type == VNODE_TYPE_EXEC && node->exec) {
            node->exec(args);
            return;
        }

        // ELF userland executables
        if (node->type == VNODE_TYPE_FILE) {
            extern int exec_elf_vnode(struct vnode *vn);
            if (exec_elf_vnode(node) == 0) {
                return; // never returns on success
            }
        }
    }

    printf("Unknown command: %s\n", cmd);
}

/* -------------------------
   Shell thread
   ------------------------- */

void shell_main(void *arg)
{
    (void)arg;


    printf("Welcome to ClearFallOS shell\n");

    char input[SHELL_LINE_MAX];
    char prompt[160];

    while (shell_running) {
        prompt[0] = '\0';
        strncpy(prompt, cwd, sizeof(prompt) - 1);
        prompt[sizeof(prompt) - 1] = '\0';

        if (strlen(prompt) + 2 < sizeof(prompt) - 1)
            strcat(prompt, "> ");

        if (readline(prompt, input, sizeof(input)) <= 0)
            continue;

        sh_add_history(input);
        sh_dispatch_line(input);
    }

    printf("Shell exiting.\n");
}