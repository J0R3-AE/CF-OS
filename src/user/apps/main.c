#include "shared/syscall.h"
#include "libc/string.h"
#include "libc/errno.h"
#include "user/shell.h"

<<<<<<< HEAD
void shell_main(void);

static const char *art =
    "###########################################################\n"
    "#                                                         #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  ###################                        ##########  #\n"
    "#  ######################        ########     ##########  #\n"
    "#  ######################  #     ###########  ##########  #\n"
    "#  ####################   ##     ###########  ##########  #\n"
    "#  ####################  ###     #######################  #\n"
    "#  ###################  ####     #######################  #\n"
    "#  ##################  #####     ########  #############  #\n"
    "#  #################  ######               #############  #\n"
    "#  ################  #######     #######   #############  #\n"
    "#  ##############                ########  #############  #\n"
    "#  #############    ########     #######################  #\n"
    "#  #############  ##########     #######################  #\n"
    "#  ###########    ##########     ##############  #######  #\n"
    "#  ###########   ###########     ############   ########  #\n"
    "#  #########    ############     ##########     ########  #\n"
    "#  #######        ########                    ##########  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#                                                         #\n"
    "###########################################################\n";

static inline void cpuid(uint32_t leaf,
                         uint32_t *eax,
                         uint32_t *ebx,
                         uint32_t *ecx,
                         uint32_t *edx)
{
    __asm__ volatile(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf));
}

static inline void cpuid_ex(uint32_t leaf, uint32_t subleaf,
                            uint32_t *eax,
                            uint32_t *ebx,
                            uint32_t *ecx,
                            uint32_t *edx)
{
    __asm__ volatile(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf));
}

static const char frames[4] = {'|', '/', '-', '\\'};

void arch_detect(uint32_t *arch, uint32_t *features)
{
    uint32_t a, b, c, d;

    // You are running x86 code → architecture is x86
    *arch = 1;

    // Check CPUID support
    uint32_t eflags;
    __asm__ volatile("pushf; pop %0" : "=r"(eflags));

    uint32_t toggled = eflags ^ (1 << 21);
    __asm__ volatile("push %0; popf" ::"r"(toggled));

    uint32_t newflags;
    __asm__ volatile("pushf; pop %0" : "=r"(newflags));

    if (((newflags ^ eflags) & (1 << 21)) == 0)
    {
        *features = 0;
        return;
    }

    // Extended CPUID leaf availability
    cpuid(0x80000000, &a, &b, &c, &d);
    uint32_t max_ext = a;

    // Detect x86_64 long mode
    if (max_ext >= 0x80000001)
    {
        cpuid(0x80000001, &a, &b, &c, &d);
        if (d & (1 << 29))
        {
            *arch = 2; // x86_64 capable
        }
    }

    // Basic feature bits
    cpuid(1, &a, &b, &c, &d);

    uint32_t f = 0;

    // Classic features
    if (d & (1 << 23))
        f |= (1 << 0); // MMX
    if (d & (1 << 25))
        f |= (1 << 1); // SSE
    if (d & (1 << 26))
        f |= (1 << 2); // SSE2

    // Extended features
    if (c & (1 << 0))
        f |= (1 << 3); // SSE3
    if (c & (1 << 9))
        f |= (1 << 4); // SSSE3
    if (c & (1 << 19))
        f |= (1 << 5); // SSE4.1
    if (c & (1 << 20))
        f |= (1 << 6); // SSE4.2
    if (c & (1 << 25))
        f |= (1 << 7); // AES
    if (c & (1 << 28))
        f |= (1 << 8); // AVX
    if (c & (1 << 23))
        f |= (1 << 9); // POPCNT
    if (c & (1 << 30))
        f |= (1 << 10); // RDRAND

    // Structured extended features (leaf 7)
    cpuid_ex(7, 0, &a, &b, &c, &d);

    if (b & (1 << 5))
        f |= (1 << 11); // AVX2
    if (b & (1 << 3))
        f |= (1 << 12); // BMI1
    if (b & (1 << 8))
        f |= (1 << 13); // BMI2
    if (b & (1 << 12))
        f |= (1 << 14); // FMA
    if (b & (1 << 18))
        f |= (1 << 15); // RDSEED
    if (b & (1 << 29))
        f |= (1 << 16); // SHA

    *features = f;
}

void print_cpu_features(uint32_t arch, uint32_t f)
{
    print("CPU Architecture: ");
    switch (arch)
    {
    case 1:
        print("x86 (32-bit)\n");
        break;
    case 2:
        print("x86_64 capable (64-bit)\n");
        break;
    default:
        print("Unknown\n");
        break;
    }

    print("Supported ISA Features:\n");

    for (int i = 0; i <= 16; i++)
    {
        if (!(f & (1 << i)))
            continue;

        switch (i)
        {
        case 0:
            print(" - MMX\n");
            break;
        case 1:
            print(" - SSE\n");
            break;
        case 2:
            print(" - SSE2\n");
            break;
        case 3:
            print(" - SSE3\n");
            break;
        case 4:
            print(" - SSSE3\n");
            break;
        case 5:
            print(" - SSE4.1\n");
            break;
        case 6:
            print(" - SSE4.2\n");
            break;
        case 7:
            print(" - AES\n");
            break;
        case 8:
            print(" - AVX\n");
            break;
        case 9:
            print(" - POPCNT\n");
            break;
        case 10:
            print(" - RDRAND\n");
            break;
        case 11:
            print(" - AVX2\n");
            break;
        case 12:
            print(" - BMI1\n");
            break;
        case 13:
            print(" - BMI2\n");
            break;
        case 14:
            print(" - FMA\n");
            break;
        case 15:
            print(" - RDSEED\n");
            break;
        case 16:
            print(" - SHA Extensions\n");
            break;
        default:
            break;
        }
    }
=======
#define SHELL_LINE_CAPACITY 256

static void print_command_error(const char *command, int error);
static int run_external(char *command, char *arguments);

static void print_help(void)
{
    printcolor("Commands:\n", MINIOS_COLOR_YELLOW, MINIOS_COLOR_BLACK);
    print("  dir [PATH] List a directory (alias: ls)\n");
    print("  cd [PATH]  Change or show the current directory\n");
    print("  pwd        Print the current directory\n");
    print("  type FILE  Print a file (alias: cat)\n");
    print("  copy SRC DST Copy a file (alias: cp)\n");
    print("  touch FILE Create an empty file\n");
    print("  mkdir DIR  Create a directory\n");
    print("  del FILE   Remove a file (alias: rm)\n");
    print("  rd DIR     Remove an empty directory (alias: rmdir)\n");
    print("  help       Show this help\n");
    print("  echo TEXT  Print TEXT\n");
    print("  color [BGFG] Set console colors using two hex digits\n");
    print("  cls        Clear the screen (alias: clear)\n");
    print("  pid        Show the current process ID\n");
    print("  ver        Show the OS version\n");
    print("  exit       Exit the shell\n");
}

static char *skip_spaces(char *text)
{
    while (*text == ' ' || *text == '\t')
        text++;
    return text;
>>>>>>> 0314c3a (	modified:   Makefile)
}

static void print_number(int value)
{
<<<<<<< HEAD
    print(art);
    uint32_t arch = 0;
    uint32_t isa = 0;
    arch_detect(&arch, &isa);
    print_cpu_features(arch, isa);
    print("Hello, World, From Usermode!\n");
    print("This is a simple demo of a usermode application.\n");
    print("It will print a spinning dial to show that it's alive.\n");

    int i = 0;
    
    while (1)
    {
        char buf[2] = {frames[i % 4], '\0'};

        print("\r");
        print(buf);

        i++;
=======
    char digits[10];
    int length = 0;
    unsigned int number;

    if (value < 0)
    {
        print("-");
        number = (unsigned int)(-(value + 1)) + 1;
>>>>>>> 0314c3a (	modified:   Makefile)
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

    printcolor("AescOS user shell\n", MINIOS_COLOR_LIGHT_CYAN,
               MINIOS_COLOR_BLACK);
    print("Type 'help' for available commands.\n");
    for (;;)
    {
        char cwd[SHELL_LINE_CAPACITY];
        int length = getcwd(cwd, sizeof(cwd));
        if (length < 0)
            print("$> ");
        else
        {
            print("$:");
            print(cwd);
            print("> ");
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