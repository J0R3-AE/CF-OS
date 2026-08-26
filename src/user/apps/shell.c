#include <stddef.h>
#include <stdint.h>

#include <syscall.h>

#define SHELL_LINE_SIZE 256
#define SHELL_MAX_ARGS 32

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

static size_t shell_strlen(const char *s)
{
    size_t len = 0;

    if (!s)
        return 0;

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

        ++a;
        ++b;
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

static void shell_trim(char *line)
{
    if (!line)
        return;

    size_t start = 0;
    size_t end = shell_strlen(line);

    while (start < end && shell_isspace(line[start]))
        ++start;

    while (end > start && shell_isspace(line[end - 1]))
        --end;

    if (start > 0)
    {
        size_t i = 0;

        while (start + i < end)
        {
            line[i] = line[start + i];
            ++i;
        }

        line[i] = '\0';
    }
    else
    {
        line[end] = '\0';
    }
}


/*
 * Parse one shell command.
 *
 * Supports:
 *
 *   hello world
 *   "hello world"
 *   'hello world'
 *   hello\ world
 *
 * '#' starts a comment when encountered outside quotes.
 *
 * The parser modifies the input buffer in-place.
 */
static int shell_parse(
    char *line,
    char **argv,
    int max_args)
{
    if (!line || !argv || max_args < 2)
        return 0;

    int argc = 0;
    char *src = line;

    while (*src && argc < max_args - 1)
    {
        while (shell_isspace(*src))
            ++src;

        if (!*src)
            break;

        /*
         * A comment begins outside quotes.
         */
        if (*src == '#')
            break;

        argv[argc++] = src;

        char *dst = src;

        int quote = 0;

        while (*src)
        {
            char c = *src;

            if (quote == 0)
            {
                if (c == '"' || c == '\'')
                {
                    quote = c;
                    ++src;
                    continue;
                }

                if (c == '\\')
                {
                    ++src;

                    if (*src)
                    {
                        *dst++ = *src++;
                        continue;
                    }

                    break;
                }

                if (c == '#')
                {
                    /*
                     * End current argument and terminate command.
                     */
                    break;
                }

                if (shell_isspace(c))
                    break;

                *dst++ = c;
                ++src;
                continue;
            }

            /*
             * Inside single/double quotes.
             */
            if (c == quote)
            {
                quote = 0;
                ++src;
                continue;
            }

            if (c == '\\' && quote == '"')
            {
                ++src;

                if (*src)
                {
                    *dst++ = *src++;
                    continue;
                }

                break;
            }

            *dst++ = c;
            ++src;
        }

        *dst = '\0';

        /*
         * Unterminated quote.
         */
        if (quote != 0)
        {
            shell_write("minios-sh: unmatched quote\n");
            return -1;
        }

        /*
         * Skip whitespace after the argument.
         */
        while (shell_isspace(*src))
            ++src;

        /*
         * Comment.
         */
        if (*src == '#')
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
        "  exit                 Exit the shell\n"
        "\n"
        "Commands:\n"
        "  command [args...]    Execute an ELF program\n"
        "\n"
        "Shell syntax:\n"
        "  \"quoted text\"        Double-quoted argument\n"
        "  'quoted text'        Single-quoted argument\n"
        "  \\                     Escape the next character\n"
        "  # comment            Ignore the rest of the line\n"
        "\n"
        "Pipes, redirection, jobs, and background execution are not\n"
        "available yet.\n"
    );
}

static void shell_echo(
    int argc,
    char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        if (i > 1)
            shell_write_char(' ');

        shell_write(argv[i]);
    }

    shell_write_char('\n');
}


/* -------------------------------------------------------------------------- */
/* Command execution                                                          */
/* -------------------------------------------------------------------------- */

static int shell_is_builtin(const char *command)
{
    if (!command)
        return 0;

    return shell_streq(command, "help") ||
           shell_streq(command, "echo") ||
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

    if (shell_streq(argv[0], "exit"))
    {
        return 1;
    }

    return 0;
}

static void shell_exec(
    int argc,
    char **argv)
{
    if (argc <= 0)
        return;

    /*
     * For now we require an executable path.
     *
     * Example:
     *
     *   /bin/test
     */
    int result =
        execve(
            argv[0],
            argv);

    /*
     * execve() should only return on failure.
     */
    if (result < 0)
    {
        shell_write("minios-sh: failed to execute: ");
        shell_write(argv[0]);
        shell_write_char('\n');
    }
}

static void shell_execute(
    int argc,
    char **argv,
    int *should_exit)
{
    if (!argc || !argv || !should_exit)
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
/* Main shell loop                                                            */
/* -------------------------------------------------------------------------- */

void shell_main(void)
{
    char line[SHELL_LINE_SIZE];

    char *argv[SHELL_MAX_ARGS];

    for (;;)
    {
        shell_write("MiniOS> ");

        int n =
            read(
                0,
                line,
                SHELL_LINE_SIZE - 1);

        /*
         * EOF.
         */
        if (n == 0)
        {
            shell_write("\n");
            break;
        }

        /*
         * Read error.
         */
        if (n < 0)
        {
            shell_write(
                "minios-sh: input error\n");

            continue;
        }

        line[n] = '\0';

        shell_trim(line);

        /*
         * Empty line.
         */
        if (!line[0])
            continue;

        int argc =
            shell_parse(
                line,
                argv,
                SHELL_MAX_ARGS);

        if (argc < 0)
            continue;

        if (argc == 0)
            continue;

        int should_exit = 0;

        shell_execute(
            argc,
            argv,
            &should_exit);

        if (should_exit)
            break;
    }

    /*
     * Your crt0 doesn't expect main().
     * Keep the shell from returning into an invalid address.
     */
    for (;;)
    {
        __asm__ volatile("hlt");
    }
}