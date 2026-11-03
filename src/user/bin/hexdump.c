#include "user/commands.h"

#include "shared/syscall.h"

static void print_character(char character)
{
    char text[2] = {character, '\0'};
    print(text);
}

static void print_hex_digit(unsigned int value)
{
    static const char digits[] = "0123456789abcdef";
    print_character(digits[value & 0xF]);
}

static void print_offset(unsigned int offset)
{
    for (int shift = 28; shift >= 0; shift -= 4)
        print_hex_digit(offset >> shift);
}

static void print_row(unsigned int offset, const unsigned char *buffer,
                      int count)
{
    print_offset(offset);
    print(": ");
    for (int i = 0; i < count; i++)
    {
        print_hex_digit(buffer[i] >> 4);
        print_hex_digit(buffer[i]);
        print(" ");
    }
    for (int i = count; i < 16; i++)
        print("   ");

    print("|");
    for (int i = 0; i < count; i++)
    {
        unsigned char character = buffer[i];
        print_character(character >= 32 && character <= 126
                            ? (char)character
                            : '.');
    }
    print("|\n");
}

int cmd_hexdump(int argc, char **argv)
{
    unsigned char buffer[16];
    unsigned char previous[16];
    unsigned int offset = 0;
    int previous_full_row = 0;
    int repeated_rows = 0;

    if (argc != 1 || !argv || !argv[0])
    {
        print("Usage: hexdump <file>\n");
        return -1;
    }

    int fd = open(argv[0], MINIOS_O_RDONLY);
    if (fd < 0)
        return fd;

    int result = 0;
    for (;;)
    {
        int count = 0;
        while (count < (int)sizeof(buffer))
        {
            int amount = read(fd, buffer + count,
                              (int)sizeof(buffer) - count);
            if (amount < 0)
            {
                result = amount;
                break;
            }
            if (amount == 0)
                break;
            count += amount;
        }
        if (result != 0)
            break;
        if (count == 0)
            break;

        int same_as_previous = count == (int)sizeof(buffer) &&
                               previous_full_row;
        if (same_as_previous)
        {
            for (int i = 0; i < count; i++)
                if (buffer[i] != previous[i])
                {
                    same_as_previous = 0;
                    break;
                }
        }

        if (same_as_previous)
            repeated_rows = 1;
        else
        {
            if (repeated_rows)
                print("*\n");
            print_row(offset, buffer, count);
            for (int i = 0; i < count; i++)
                previous[i] = buffer[i];
            previous_full_row = count == (int)sizeof(buffer);
            repeated_rows = 0;
        }
        offset += (unsigned int)count;
    }

    if (repeated_rows)
        print("*\n");
    print_offset(offset);
    print("\n");

    int close_result = close(fd);
    return result != 0 ? result : close_result;
}
