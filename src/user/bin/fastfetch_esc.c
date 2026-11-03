#include "user/commands.h"

#include "shared/syscall.h"

static void print_info(const char *label, const char *value,
                       int value_color)
{
    printcolor(label, MINIOS_COLOR_LIGHT_CYAN, MINIOS_COLOR_BLACK);
    printcolor(value, value_color, MINIOS_COLOR_BLACK);
    print("\n");
}

static void print_number(unsigned int value)
{
    char digits[10];
    int count = 0;
    do
    {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value && count < (int)sizeof(digits));

    while (count)
    {
        char digit[2] = {digits[--count], '\0'};
        printcolor(digit, MINIOS_COLOR_LIGHT_GREEN, MINIOS_COLOR_BLACK);
    }
}

int cmd_fastfetch_esc(int argc, char **argv)
{
    char cwd[128];
    (void)argv;

    if (argc != 0)
    {
        print("Usage: fastfetch-esc\n");
        return -1;
    }

    if (getcwd(cwd, sizeof(cwd)) < 0)
        cwd[0] = '\0';

    printcolor("       .----.      ", MINIOS_COLOR_YELLOW, MINIOS_COLOR_BLACK);
    print_info("OS:       ", "Clearfall / AescOS", MINIOS_COLOR_LIGHT_GREEN);
    printcolor("      / .--. \\     ", MINIOS_COLOR_YELLOW, MINIOS_COLOR_BLACK);
    print_info("Kernel:   ", "i386", MINIOS_COLOR_LIGHT_GREEN);
    printcolor("     | |    | |    ", MINIOS_COLOR_YELLOW, MINIOS_COLOR_BLACK);
    print_info("Shell:    ", "AescOS", MINIOS_COLOR_LIGHT_GREEN);
    printcolor("      \\ '--' /     ", MINIOS_COLOR_YELLOW, MINIOS_COLOR_BLACK);
    printcolor("PID:      ", MINIOS_COLOR_LIGHT_CYAN, MINIOS_COLOR_BLACK);
    print_number((unsigned int)getpid());
    print("\n");
    printcolor("       '----'      ", MINIOS_COLOR_YELLOW, MINIOS_COLOR_BLACK);
    print_info("Directory:", cwd[0] ? cwd : "(unavailable)",
               MINIOS_COLOR_LIGHT_GREEN);
    return 0;
}
