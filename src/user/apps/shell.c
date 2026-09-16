#include <stddef.h>
#include <stdint.h>

#include <syscall.h>

#define SHELL_LINE_SIZE 256
#define SHELL_MAX_ARGS  32

/* -------------------------------------------------------------------------- */
/* Output                                                                     */
/* -------------------------------------------------------------------------- */

static void shell_write(const char *s)
{
    if (!s)
        return;

    size_t len = 0;

    while (s[len])
        len++;

    if (len)
        write(1, s, len);
}

static void shell_write_char(char c)
{
    write(1, &c, 1);
}

/* -------------------------------------------------------------------------- */
/* String helpers                                                             */
/* -------------------------------------------------------------------------- */

static size_t shell_strlen(const char *s)
{
    if (!s)
        return 0;

    size_t len = 0;

    while (s[len])
        len++;

    return len;
}

static int shell_streq(const char *a, const char *b)
{
    if (!a || !b)
        return 0;

    while (*a && *b)
    {
        if (*a != *b)
            return 0;

        a++;
        b++;
    }

    return *a == '\0' && *b == '\0';
}

static int shell_isspace(char c)
{
    return c == ' '  ||
           c == '\t' ||
           c == '\n' ||
           c == '\r';
}

/* -------------------------------------------------------------------------- */
/* Line handling                                                              */
/* -------------------------------------------------------------------------- */

static void shell_trim(char *line)
{
    if (!line)
        return;

    size_t start = 0;
    size_t end = shell_strlen(line);

    while (start < end && shell_isspace(line[start]))
        start++;

    while (end > start && shell_isspace(line[end - 1]))
        end--;

    size_t len = end - start;

    if (start != 0)
    {
        size_t i;

        for (i = 0; i < len; i++)
            line[i] = line[start + i];
    }

    line[len] = '\0';
}

/* -------------------------------------------------------------------------- */
/* Command parser                                                             */
/* -------------------------------------------------------------------------- */

/*
 * Supports:
 *
 *   hello world
 *   "hello world"
 *   'hello world'
 *   hello\ world
 *   echo hello # comment
 */

static int shell_parse(
    char *line,
    char **argv,
    int max_args)
{
    if (!line || !argv || max_args < 2)
        return 0;

    int argc = 0;
    char *p = line;

    while (*p)
    {
        /* Skip whitespace */
        while (shell_isspace(*p))
            p++;

        if (!*p)
            break;

        /* Comment */
        if (*p == '#')
            break;

        if (argc >= max_args - 1)
            break;

        argv[argc++] = p;

        char *out = p;
        char quote = 0;

        while (*p)
        {
            /* Inside quotes */
            if (quote)
            {
                if (*p == quote)
                {
                    quote = 0;
                    p++;
                    continue;
                }

                if (*p == '\\' && p[1])
                {
                    p++;
                    *out++ = *p++;
                    continue;
                }

                *out++ = *p++;
                continue;
            }

            /* Start quote */
            if (*p == '"' || *p == '\'')
            {
                quote = *p;
                p++;
                continue;
            }

            /* Escape next character */
            if (*p == '\\')
            {
                if (p[1])
                {
                    p++;
                    *out++ = *p++;
                }
                else
                {
                    p++;
                }

                continue;
            }

            /* Comment */
            if (*p == '#')
                break;

            /* Argument separator */
            if (shell_isspace(*p))
                break;

            *out++ = *p++;
        }

        if (quote)
        {
            shell_write("minios-sh: unmatched quote\n");
            return -1;
        }

        *out = '\0';

        /* Skip whitespace before the next argument */
        while (shell_isspace(*p))
            p++;

        /* Ignore rest of line after comment */
        if (*p == '#')
            break;
    }

    argv[argc] = NULL;
    return argc;
}

/* -------------------------------------------------------------------------- */
/* Builtins                                                                   */
/* -------------------------------------------------------------------------- */

static void shell_help(void)
{
    shell_write(
        "MiniOS shell\n"
        "\n"
        "Builtins:\n"
        "  help                 Show this help\n"
        "  echo [args...]       Print arguments\n"
        "  cd <dir>             Change directory\n"
        "  ls [dir]             List directory\n"
        "  exit                 Exit shell\n"
        "\n"
        "Commands:\n"
        "  command [args...]    Execute an ELF program\n"
        "\n"
        "Syntax:\n"
        "  \"quoted text\"        Double quotes\n"
        "  'quoted text'        Single quotes\n"
        "  \\                     Escape character\n"
        "  # comment            Comment\n"
        "\n"
    );
}

static void shell_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (i > 1)
            shell_write_char(' ');

        shell_write(argv[i]);
    }

    shell_write_char('\n');
}

static void shell_cd(int argc, char **argv)
{
    if (argc < 2)
    {
        shell_write("minios-sh: cd: missing operand\n");
        return;
    }

    int r = chdir(argv[1]);

    if (r < 0)
    {
        shell_write("minios-sh: cd: failed\n");
    }
}

static void shell_ls(int argc, char **argv)
{
    const char *path = ".";

    if (argc >= 2)
        path = argv[1];

    int fd = open(path, VFS_O_RDONLY);

    if (fd < 0)
    {
        shell_write("minios-sh: ls: failed to open directory\n");
        return;
    }

    dirent_t d;
    int index = 0;

    while (readdir(fd, index, &d) == 0)
    {
        shell_write(d.name);
        shell_write_char('\n');

        index++;
    }

    close(fd);
}

/* -------------------------------------------------------------------------- */
/* Builtin dispatch                                                           */
/* -------------------------------------------------------------------------- */

static int shell_is_builtin(const char *command)
{
    if (!command)
        return 0;

    return shell_streq(command, "help") ||
           shell_streq(command, "echo") ||
           shell_streq(command, "cd")   ||
           shell_streq(command, "ls")   ||
           shell_streq(command, "exit");
}

static int shell_builtin(
    int argc,
    char **argv)
{
    if (argc <= 0)
        return 0;

    if (shell_streq(argv[0], "help"))
    {
        shell_help();
        return 0;
    }

    if (shell_streq(argv[0], "echo"))
    {
        shell_echo(argc, argv);
        return 0;
    }

    if (shell_streq(argv[0], "cd"))
    {
        shell_cd(argc, argv);
        return 0;
    }

    if (shell_streq(argv[0], "ls"))
    {
        shell_ls(argc, argv);
        return 0;
    }

    if (shell_streq(argv[0], "exit"))
    {
        return 1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
/* External command execution                                                */
/* -------------------------------------------------------------------------- */

static void shell_exec(
    int argc,
    char **argv)
{
    if (argc <= 0)
        return;

    /*
     * Try exactly what the user typed first.
     */
    int result = execve(argv[0], argv);

    if (result >= 0)
        return;

    /*
     * Try /bin/<command> for bare commands.
     */
    if (argv[0][0] != '/')
    {
        char path[256];

        size_t n = 0;

        path[n++] = '/';
        path[n++] = 'b';
        path[n++] = 'i';
        path[n++] = 'n';
        path[n++] = '/';

        size_t i = 0;

        while (argv[0][i] &&
               n + 1 < sizeof(path))
        {
            path[n++] = argv[0][i++];
        }

        path[n] = '\0';

        result = execve(path, argv);

        if (result >= 0)
            return;
    }

    shell_write("minios-sh: command not found: ");
    shell_write(argv[0]);
    shell_write_char('\n');
}

/* -------------------------------------------------------------------------- */
/* Execute command                                                            */
/* -------------------------------------------------------------------------- */

static void shell_execute(
    int argc,
    char **argv,
    int *should_exit)
{
    if (!argv || !should_exit || argc <= 0)
        return;

    if (shell_is_builtin(argv[0]))
    {
        if (shell_builtin(argc, argv))
            *should_exit = 1;

        return;
    }

    shell_exec(argc, argv);
}

/* -------------------------------------------------------------------------- */
/* Main                                                                       */
/* -------------------------------------------------------------------------- */

void shell_main(void)
{
    char line[SHELL_LINE_SIZE];
    char *argv[SHELL_MAX_ARGS];

    for (;;)
    {
        /*
         * Always print the prompt before waiting for input.
         */
        shell_write("MiniOS> ");

        /* Debug: indicate we're waiting for input */
        shell_write("[shell-debug] waiting for input\n");

        int n = read(0, line, SHELL_LINE_SIZE - 1);

        /* Debug: raw input received */
        shell_write("[shell-debug] raw: '");
        shell_write(line);
        shell_write("'\n");

        /*
         * EOF / shell terminated.
         */
        if (n == 0)
        {
            shell_write_char('\n');
            break;
        }

        /*
         * Input error.
         */
        if (n < 0)
        {
            shell_write(
                "minios-sh: input error\n");

            continue;
        }

        /*
         * Always terminate the line ourselves.
         */
        if ((size_t)n >= SHELL_LINE_SIZE)
            n = SHELL_LINE_SIZE - 1;

        line[n] = '\0';

        /*
         * Remove the newline / surrounding whitespace.
         */
        shell_trim(line);

        /*
         * Empty command.
         */
        if (!line[0])
            continue;

        /*
         * Parse.
         */
        int argc = shell_parse(
            line,
            argv,
            SHELL_MAX_ARGS);

        if (argc < 0)
            continue;

        if (argc == 0)
            continue;

        /*
         * Execute.
         */
        int should_exit = 0;

        shell_execute(
            argc,
            argv,
            &should_exit);

        if (should_exit)
            break;
    }

    /*
     * shell_main should never return into an invalid caller.
     */
    for (;;)
    {
        __asm__ volatile("hlt");
    }
}