// src/user/apps/sh.c - Full-featured Shell

#include "../../libc/syscall.h"
#include "../../libc/dirent.h"
#include <stdarg.h>
#include <stddef.h>

// Standard library functions
int printf(const char *fmt, ...);
int getchar(void);
int strcmp(const char *s1, const char *s2);
int strlen(const char *s);
void *memcpy(void *dest, const void *src, int n);
void *memset(void *s, int c, int n);
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);
int strncmp(const char *s1, const char *s2, int n);
char *strchr(const char *s, int c);

void exit(int status);

// File/syscall wrappers from your libc
int open(const char *path, int flags, int mode);
int close(int fd);
int read(int fd, void *buf, int count);
int write(int fd, const void *buf, int count);

// exec syscall wrapper
static int sys_execve(const char *path, char **argv)
{
    return syscall(SYS_execve, (int)path, (int)argv, 0);
}

#define CMD_LEN 512
#define MAX_ARGS 32
#define HISTORY_SIZE 20
#define COMMAND_COUNT ((int)(sizeof(g_commands) / sizeof(g_commands[0])))

typedef struct
{
    char command[CMD_LEN];
} history_t;

typedef int (*cmd_fn_t)(int argc, char **argv);

typedef struct
{
    const char *name;
    cmd_fn_t fn;
    const char *help;
} command_t;

// Global state
static int g_shell_running = 1;
static char g_cwd[256] = "/";
static history_t g_history[HISTORY_SIZE];
static int g_history_count = 0;

///

static cmd_echo(int argc, char **argv);
static cmd_exit(int argc, char **argv);
static cmd_pwd(int argc, char **argv);
static cmd_cd(int argc, char **argv);
static cmd_ls(int argc, char **argv);
static cmd_cat(int argc, char **argv);
static cmd_touch(int argc, char **argv);
static cmd_history(int argc, char **argv);
static cmd_clear(int argc, char **argv);
static cmd_date(int argc, char **argv);
static cmd_whoami(int argc, char **argv);
static cmd_uname(int argc, char **argv);
static cmd_info(int argc, char **argv);
static cmd_exec(int argc, char **argv);
static cmd_help(int argc, char **argv);

// ============================================================================
// Command table
// ============================================================================

static const command_t g_commands[] = {
    {"echo", cmd_echo, "echo <args...>"},
    {"exit", cmd_exit, "exit"},
    {"help", cmd_help, "help"},
    {"pwd", cmd_pwd, "pwd"},
    {"cd", cmd_cd, "cd <dir>"},
    {"ls", cmd_ls, "ls [dir]"},
    {"cat", cmd_cat, "cat <file>"},
    {"touch", cmd_touch, "touch <file>"},
    {"history", cmd_history, "history"},
    {"clear", cmd_clear, "clear"},
    {"date", cmd_date, "date"},
    {"whoami", cmd_whoami, "whoami"},
    {"uname", cmd_uname, "uname"},
    {"info", cmd_info, "info"},
};

// ============================================================================
// Builtin Commands
// ============================================================================

static int cmd_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        printf("%s", argv[i]);
        if (i < argc - 1)
            printf(" ");
    }
    printf("\n");
    return 0;
}

static int cmd_exit(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("Exiting shell...\n");
    g_shell_running = 0;
    return 0;
}

static int cmd_pwd(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("%s\n", g_cwd);
    return 0;
}

static int cmd_cd(int argc, char **argv)
{
    if (argc < 2)
    {
        strcpy(g_cwd, "/");
        printf("Directory: %s\n", g_cwd);
        return 0;
    }

    if (strcmp(argv[1], "/") == 0)
    {
        strcpy(g_cwd, "/");
    }
    else if (strcmp(argv[1], "..") == 0)
    {
        int len = strlen(g_cwd);
        if (len > 1)
        {
            for (int i = len - 1; i >= 0; i--)
            {
                if (g_cwd[i] == '/')
                {
                    if (i == 0)
                        g_cwd[1] = '\0';
                    else
                        g_cwd[i] = '\0';
                    break;
                }
            }
        }
    }
    else
    {
        int len = strlen(g_cwd);
        if (len > 1 && g_cwd[len - 1] != '/')
            strcat(g_cwd, "/");

        strcat(g_cwd, argv[1]);
    }

    printf("Directory: %s\n", g_cwd);
    return 0;
}

static int cmd_cat(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: cat <file>\n");
        return -1;
    }

    int fd = open(argv[1], 0, 0);
    if (fd < 0)
    {
        printf("cat: cannot open %s\n", argv[1]);
        return -1;
    }

    char buf[512];
    int n;

    while ((n = read(fd, buf, sizeof(buf))) > 0)
        write(1, buf, n);

    close(fd);
    return 0;
}

static int cmd_touch(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("usage: touch <file>\n");
        return -1;
    }

    // Use your VFS create flag: 0x1 == VFS_O_CREATE
    int fd = syscall(SYS_open, (int)argv[1], 0x1, 0);
    if (fd < 0)
    {
        printf("touch: failed to create %s\n", argv[1]);
        return -1;
    }

    syscall(SYS_close, fd, 0, 0);
    return 0;
}

static int cmd_ls(int argc, char **argv)
{
    const char *path = "/";

    printf("MiniOS ls - list directory contents\n");

    if (argc > 1)
        path = argv[1];

    int fd = syscall(SYS_open, (int)path, 0, 0);
    if (fd < 0)
    {
        printf("ls: failed to open %s\n", path);
        return -1;
    }

    printf("Listing %s:\n", path);

    for (int i = 0;; i++)
    {
        dirent_t ent;

        int r = syscall(SYS_readdir, fd, i, (int)&ent);
        if (r != 0)
            break;

        printf("  %s\n", ent.name);
    }

    syscall(SYS_close, fd, 0, 0);
    return 0;
}

static int cmd_history(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (g_history_count == 0)
    {
        printf("history: empty\n");
        return 0;
    }

    int start = 0;
    if (g_history_count > HISTORY_SIZE)
        start = g_history_count - HISTORY_SIZE;

    for (int i = 0; i < g_history_count && i < HISTORY_SIZE; i++)
    {
        printf("%d  %s\n", i + 1, g_history[i].command);
    }

    return 0;
}

static int cmd_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("\033[2J\033[H");
    return 0;
}

static int cmd_date(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("date: not implemented yet\n");
    return 0;
}

static int cmd_whoami(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("root\n");
    return 0;
}

static int cmd_uname(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("MiniOS\n");
    return 0;
}

static int cmd_info(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("MiniOS Shell v1.0\n");
    printf("cwd: %s\n", g_cwd);
    printf("history entries: %d\n", g_history_count);
    return 0;
}

// ============================================================================
// Command parsing
// ============================================================================

static void parse_command(char *line, int *argc, char **argv)
{
    *argc = 0;

    while (*line && *argc < MAX_ARGS - 1)
    {
        while (*line == ' ' || *line == '\t')
            line++;

        if (!*line)
            break;

        argv[*argc] = line;
        (*argc)++;

        while (*line && *line != ' ' && *line != '\t')
            line++;

        if (*line)
        {
            *line = '\0';
            line++;
        }
    }

    argv[*argc] = 0;
}

// ============================================================================
// Builtin dispatch
// ============================================================================

static int cmd_help(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("=== Available Commands ===\n");
    for (int i = 0; i < COMMAND_COUNT; i++)
        printf("%s\n", g_commands[i].name);

    return 0;
}

static int run_builtin(int argc, char **argv)
{
    if (!argv[0])
        return -1;

    for (int i = 0; i < COMMAND_COUNT; i++)
    {
        if (strcmp(argv[0], g_commands[i].name) == 0)
            return g_commands[i].fn(argc, argv);
    }

    return -1;
}

// ============================================================================
// History
// ============================================================================

static void add_history(const char *cmd)
{
    if (g_history_count < HISTORY_SIZE)
    {
        strcpy(g_history[g_history_count++].command, cmd);
    }
    else
    {
        for (int i = 0; i < HISTORY_SIZE - 1; i++)
            strcpy(g_history[i].command, g_history[i + 1].command);

        strcpy(g_history[HISTORY_SIZE - 1].command, cmd);
    }
}

// ============================================================================
// Shell main
// ============================================================================

void sh_main(void)
{
    printf("\nMiniOS Shell v1.0\n\n");

    g_shell_running = 1;

    while (g_shell_running)
    {
        char cmd_buf[CMD_LEN];
        int pos = 0;

        printf("[root@minios]$ ");

        while (1)
        {
            int ch = getchar();

            if (ch < 0)
                continue;

            if (ch == '\n' || ch == '\r')
            {
                printf("\n");
                break;
            }

            if (pos < CMD_LEN - 1)
                cmd_buf[pos++] = (char)ch;
        }

        cmd_buf[pos] = 0;

        if (pos == 0)
            continue;

        add_history(cmd_buf);

        char *argv[MAX_ARGS];
        int argc = 0;

        parse_command(cmd_buf, &argc, argv);

        if (argc <= 0)
            continue;

        // 1. try builtin
        int ret = run_builtin(argc, argv);

        // 2. fallback to external program
        if (ret < 0)
        {
            char path[128];

            strcpy(path, "/bin/");
            strcat(path, argv[0]);

            int exec_ret = sys_execve(path, argv);

            if (exec_ret < 0)
                printf("Command not found: %s\n", argv[0]);
        }
    }

    printf("Shell exited.\n");
}