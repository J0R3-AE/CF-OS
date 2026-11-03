#include "shared/syscall.h"
#include "libc/string.h"
#include "libc/errno.h"
#include "user/shell.h"
#include <stddef.h>

#define SHELL_LINE_CAPACITY 256

static void print_command_error(const char *command, int error);
static int run_external(char *command, char *arguments);

static void print_help_item(const char *command, const char *description)
{
    printcolor("  ", MINIOS_COLOR_LIGHT_CYAN, MINIOS_COLOR_BLACK);
    printcolor(command, MINIOS_COLOR_LIGHT_CYAN, MINIOS_COLOR_BLACK);
    for (size_t i = strlen(command); i < 14; i++)
        print(" ");
    print(description);
    print("\n");
}

static void print_help_section(const char *title)
{
    print("\n");
    printcolor(title, MINIOS_COLOR_YELLOW, MINIOS_COLOR_BLACK);
    print("\n");
}

static void print_help(void)
{
    printcolor("Commands\n", MINIOS_COLOR_YELLOW, MINIOS_COLOR_BLACK);
    printcolor("---------------------------------------------\n",
               MINIOS_COLOR_DARK_GREY, MINIOS_COLOR_BLACK);

    print_help_section("Navigation");
    print_help_item("cd [PATH]", "Change directory; no path shows current directory");
    print_help_item("pwd", "Show the current directory");
    print_help_item("dir [PATH]", "List a directory (alias: ls)");

    print_help_section("Files");
    print_help_item("type FILE", "Print a file (alias: cat)");
    print_help_item("more FILE", "View a file one page at a time");
    print_help_item("hexdump FILE", "Display file bytes in hexadecimal");
    print_help_item("copy SRC DST", "Copy a file (alias: cp)");
    print_help_item("touch FILE", "Create an empty file");
    print_help_item("mkdir DIR", "Create a directory");
    print_help_item("del FILE", "Remove a file (alias: rm)");
    print_help_item("rd DIR", "Remove an empty directory (alias: rmdir)");

    print_help_section("Shell");
    print_help_item("help", "Show this help");
    print_help_item("echo TEXT", "Print text");
    print_help_item("color [BGFG]", "Set console colors using two hex digits");
    print_help_item("cls", "Clear the screen (alias: clear)");
    print_help_item("pid", "Show the current process ID");
    print_help_item("ver", "Show the OS version");
    print_help_item("fastfetch-esc", "Show system information");
    print_help_item("exit", "Exit the shell");
}

static char *skip_spaces(char *text)
{
    while (*text == ' ' || *text == '\t')
        text++;
    return text;
}

static void print_number(int value)
{
    char digits[10];
    int length = 0;
    unsigned int number;

    if (value < 0)
    {
        print("-");
        number = (unsigned int)(-(value + 1)) + 1;
    }
    else
        number = (unsigned int)value;

    do
    {
        digits[length++] = (char)('0' + number % 10);
        number /= 10;
    } while (number && length < (int)sizeof(digits));

    while (length)
    {
        char digit[2] = {digits[--length], '\0'};
        print(digit);
    }
}

static int parse_path(char *arguments, char **path)
{
    arguments = skip_spaces(arguments);
    if (!arguments[0])
        return -1;

    char *end = arguments;
    while (*end && *end != ' ' && *end != '\t')
        end++;
    if (*end)
    {
        *end++ = '\0';
        if (skip_spaces(end)[0])
            return -1;
    }

    *path = arguments;
    return 0;
}

static int parse_hex_digit(char digit)
{
    if (digit >= '0' && digit <= '9')
        return digit - '0';
    if (digit >= 'a' && digit <= 'f')
        return digit - 'a' + 10;
    if (digit >= 'A' && digit <= 'F')
        return digit - 'A' + 10;
    return -1;
}

static void command_color(char *arguments)
{
    arguments = skip_spaces(arguments);
    char *end = arguments;
    while (*end)
        end++;
    while (end > arguments && (end[-1] == ' ' || end[-1] == '\t'))
        *--end = '\0';

    if (!arguments[0])
    {
        int result = setcolor(MINIOS_COLOR_LIGHT_GREY, MINIOS_COLOR_BLACK);
        if (result < 0)
            print_command_error("color", result);
        return;
    }

    if (arguments[1] == '\0' || arguments[2] != '\0')
    {
        print("Usage: color [BGFG] (two hexadecimal digits)\n");
        return;
    }

    int background = parse_hex_digit(arguments[0]);
    int foreground = parse_hex_digit(arguments[1]);
    if (background < 0 || foreground < 0)
    {
        print("Color must use hexadecimal digits 0-F.\n");
        return;
    }

    int result = setcolor(foreground, background);
    if (result < 0)
        print_command_error("color", result);
}

static void print_command_error(const char *command, int error)
{
    printcolor(command, MINIOS_COLOR_LIGHT_RED, MINIOS_COLOR_BLACK);
    print(" failed (error ");
    print_number(error);
    print(").\n");
}

static const char *external_command_path(const char *command)
{
    if (strcasecmp(command, "dir") == 0 || strcasecmp(command, "ls") == 0)
        return "/bin/dir";
    if (strcasecmp(command, "type") == 0 || strcasecmp(command, "cat") == 0)
        return "/bin/type";
    if (strcasecmp(command, "more") == 0)
        return "/bin/more";
    if (strcasecmp(command, "hexdump") == 0 || strcasecmp(command, "hex") == 0)
        return "/bin/hexdump";
    if (strcasecmp(command, "copy") == 0 || strcasecmp(command, "cp") == 0)
        return "/bin/copy";
    if (strcasecmp(command, "touch") == 0)
        return "/bin/touch";
    if (strcasecmp(command, "mkdir") == 0)
        return "/bin/mkdir";
    if (strcasecmp(command, "del") == 0 || strcasecmp(command, "rm") == 0)
        return "/bin/del";
    if (strcasecmp(command, "rd") == 0 ||
        strcasecmp(command, "rmdir") == 0)
        return "/bin/rd";
    if (strcasecmp(command, "ver") == 0)
        return "/bin/ver";
    if (strcasecmp(command, "fastfetch-esc") == 0 ||
        strcasecmp(command, "fastfetch") == 0)
        return "/bin/fastfetch-esc";
    return NULL;
}

static int run_external(char *command, char *arguments)
{
    const char *path = external_command_path(command);
    if (!path)
        return ERR_NOT_FOUND;

    char *argv[16];
    int argc = 1;
    argv[0] = command;
    char *cursor = skip_spaces(arguments);
    while (*cursor)
    {
        if (argc == (int)(sizeof(argv) / sizeof(argv[0])) - 1)
        {
            print("Too many command arguments.\n");
            return ERR_BUFFER_TOO_SMALL;
        }
        argv[argc++] = cursor;
        while (*cursor && *cursor != ' ' && *cursor != '\t')
            cursor++;
        if (*cursor)
        {
            *cursor++ = '\0';
            cursor = skip_spaces(cursor);
        }
    }
    argv[argc] = NULL;

    int pid = execve(path, argv);
    if (pid < 0)
        return pid;

    int status = 0;
    int waited = waitpid(pid, &status, 0);
    if (waited < 0)
        return waited;
    if (status != 0)
        print_command_error(command, status);
    return ERR_SUCCESS;
}

static void run_command(char *line)
{
    char *command = skip_spaces(line);
    char *arguments = command;

    while (*arguments && *arguments != ' ' && *arguments != '\t')
        arguments++;

    if (*arguments)
    {
        *arguments++ = '\0';
        arguments = skip_spaces(arguments);
    }

    if (!command[0])
        return;

    if (strcasecmp(command, "help") == 0)
        print_help();
    else if (strcasecmp(command, "pwd") == 0)
    {
        char path[SHELL_LINE_CAPACITY];
        int length = getcwd(path, sizeof(path));
        if (length < 0)
            print_command_error("pwd", length);
        else
        {
            printcolor(path, MINIOS_COLOR_LIGHT_CYAN, MINIOS_COLOR_BLACK);
            print("\n");
        }
    }
    else if (strcasecmp(command, "cd") == 0)
    {
        if (!arguments[0])
        {
            char path[SHELL_LINE_CAPACITY];
            int length = getcwd(path, sizeof(path));
            if (length < 0)
                print_command_error("cd", length);
            else
            {
                printcolor(path, MINIOS_COLOR_LIGHT_CYAN,
                           MINIOS_COLOR_BLACK);
                print("\n");
            }
            return;
        }

        char *path;
        if (parse_path(arguments, &path) != 0)
        {
            print("Usage: cd [path]\n");
            return;
        }
        int result = chdir(path);
        if (result < 0)
            print_command_error("cd", result);
    }
    else if (strcasecmp(command, "echo") == 0)
    {
        print(arguments);
        print("\n");
    }
    else if (strcasecmp(command, "color") == 0)
        command_color(arguments);
    else if (strcasecmp(command, "clear") == 0 ||
             strcasecmp(command, "cls") == 0)
        clear_screen();
    else if (strcasecmp(command, "pid") == 0)
    {
        print_number(getpid());
        print("\n");
    }
    else if (strcasecmp(command, "exit") == 0)
        exit(0);
    else if (external_command_path(command))
    {
        int result = run_external(command, arguments);
        if (result < 0)
            print_command_error(command, result);
    }
    else
    {
        print("Unknown command: ");
        print(command);
        print("\nType 'help' to see available commands.\n");
    }
}

int shell(void)
{
    char line[SHELL_LINE_CAPACITY];

    printcolor("\n  AescOS Shell\n", MINIOS_COLOR_LIGHT_CYAN,
               MINIOS_COLOR_BLACK);
    printcolor("  Simple tools for files and directories\n",
               MINIOS_COLOR_DARK_GREY, MINIOS_COLOR_BLACK);
    print("\n");
    for (;;)
    {
        char cwd[SHELL_LINE_CAPACITY];
        int length = getcwd(cwd, sizeof(cwd));
        if (length < 0)
        {
            printcolor("$:", MINIOS_COLOR_LIGHT_CYAN, MINIOS_COLOR_BLACK);
            printcolor("> ", MINIOS_COLOR_YELLOW, MINIOS_COLOR_BLACK);
        }
        else
        {
            printcolor("$:", MINIOS_COLOR_LIGHT_CYAN, MINIOS_COLOR_BLACK);
            printcolor(cwd, MINIOS_COLOR_LIGHT_GREEN, MINIOS_COLOR_BLACK);
            printcolor("> ", MINIOS_COLOR_YELLOW, MINIOS_COLOR_BLACK);
        }
        int result = scan(line, sizeof(line));
        if (result < 0)
        {
            print("Input line is too long.\n");
            continue;
        }

        run_command(line);
    }
}

int main(void)
{
    return shell();
}