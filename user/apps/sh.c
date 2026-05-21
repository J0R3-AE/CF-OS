// user/apps/sh.c - Simple shell

int printf(const char *fmt, ...);
int getchar(void);
void exit(int status);
int strcmp(const char *s1, const char *s2);

#define CMD_LEN 256
#define MAX_ARGS 16

typedef struct {
    char name[64];
    int (*func)(int argc, char **argv);
} builtin_cmd_t;

/* -------------------------------------------------------------------------- */
/* Built-in commands                                                          */
/* -------------------------------------------------------------------------- */

int cmd_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);

        if (i < argc - 1)
            printf(" ");
    }

    printf("\n");
    return 0;
}

int cmd_exit(int argc, char **argv)
{
    int code = 0;

    if (argc > 1)
        code = argv[1][0] - '0';

    exit(code);
    return 0;
}

int cmd_help(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("Available commands:\n");
    printf("  echo - print text\n");
    printf("  exit - exit shell\n");
    printf("  help - show this help\n");

    return 0;
}

/* -------------------------------------------------------------------------- */
/* Command dispatcher                                                         */
/* -------------------------------------------------------------------------- */

int find_and_run_builtin(int argc, char **argv)
{
    if (argc <= 0 || !argv[0])
        return -1;

    if (strcmp(argv[0], "echo") == 0)
        return cmd_echo(argc, argv);

    if (strcmp(argv[0], "exit") == 0)
        return cmd_exit(argc, argv);

    if (strcmp(argv[0], "help") == 0)
        return cmd_help(argc, argv);

    return -1;
}

/* -------------------------------------------------------------------------- */
/* Parser                                                                     */
/* -------------------------------------------------------------------------- */

void parse_command(char *line, int *argc, char **argv)
{
    *argc = 0;

    while (*line && *argc < MAX_ARGS) {

        /* Skip whitespace */
        while (*line == ' ' || *line == '\t')
            line++;

        if (*line == '\0' || *line == '\n')
            break;

        argv[*argc] = line;
        (*argc)++;

        /* Find end of token */
        while (*line &&
               *line != ' ' &&
               *line != '\t' &&
               *line != '\n')
        {
            line++;
        }

        if (*line) {
            *line = '\0';
            line++;
        }
    }

    argv[*argc] = 0;
}

/* -------------------------------------------------------------------------- */
/* Main shell                                                                 */
/* -------------------------------------------------------------------------- */

int sh_main(void)
{
    printf("Simple Shell (v0.1)\n");
    printf("Type 'help' for available commands\n");

    while (1) {

        printf("$ ");

        char cmd_buf[CMD_LEN];
        int pos = 0;

        while (1) {

            int ch = getchar();

            if (ch < 0)
                continue;

            /* Enter */
            if (ch == '\n' || ch == '\r') {

                cmd_buf[pos] = '\0';
                printf("\n");
                break;
            }

            /* Backspace */
            else if (ch == '\b' || ch == 127) {

                if (pos > 0) {
                    pos--;
                    printf("\b \b");
                }
            }

            /* Normal character */
            else {

                if (pos < CMD_LEN - 1) {
                    cmd_buf[pos++] = (char)ch;

                    /* Echo character */
                    printf("%c", ch);
                }
            }
        }

        if (pos == 0)
            continue;

        int argc;
        char *argv[MAX_ARGS + 1];

        parse_command(cmd_buf, &argc, argv);

        if (argc == 0)
            continue;

        int ret = find_and_run_builtin(argc, argv);

        if (ret < 0)
            printf("Command not found: %s\n", argv[0]);
    }

    return 0;
}