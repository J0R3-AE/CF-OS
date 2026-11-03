#include "user/commands.h"

#include "shared/syscall.h"
#include "libc/mem.h"
#include "libc/string.h"

#define COPY_PATH_CAPACITY 256

static int normalize_path(const char *path, const char *cwd,
                          char *normalized, usize capacity)
{
    usize output_length;

    if (!path || !normalized || capacity < 2)
        return -1;

    if (path[0] == '/')
    {
        normalized[0] = '/';
        normalized[1] = '\0';
        output_length = 1;
    }
    else
    {
        if (!cwd || cwd[0] != '/')
            return -1;
        output_length = strlen(cwd);
        if (output_length >= capacity)
            return -1;
        memcpy(normalized, cwd, output_length + 1);
    }

    while (*path)
    {
        while (*path == '/')
            path++;
        if (!*path)
            break;

        const char *component = path;
        usize component_length = 0;
        while (path[component_length] && path[component_length] != '/')
            component_length++;
        path += component_length;

        if (component_length == 1 && component[0] == '.')
            continue;
        if (component_length == 2 && component[0] == '.' &&
            component[1] == '.')
        {
            if (output_length > 1)
            {
                while (output_length > 1 &&
                       normalized[output_length - 1] != '/')
                    output_length--;
                if (output_length > 1)
                    output_length--;
                normalized[output_length] = '\0';
            }
            continue;
        }

        usize separator_length = output_length > 1 ? 1 : 0;
        if (output_length + separator_length + component_length + 1 >
            capacity)
            return -1;

        if (separator_length)
            normalized[output_length++] = '/';
        memcpy(normalized + output_length, component, component_length);
        output_length += component_length;
        normalized[output_length] = '\0';
    }

    return 0;
}

int cmd_copy(int argc, char **argv)
{
    char buffer[256];
    char cwd[COPY_PATH_CAPACITY];
    char normalized_source[COPY_PATH_CAPACITY];
    char normalized_destination[COPY_PATH_CAPACITY];
    int input;
    int output;
    int result = 0;

    if (argc != 2 || !argv || !argv[0] || !argv[1])
    {
        print("Usage: copy <source> <destination>\n");
        return -1;
    }

    int cwd_result = getcwd(cwd, sizeof(cwd));
    if (cwd_result < 0)
        return cwd_result;

    if (normalize_path(argv[0], cwd, normalized_source,
                       sizeof(normalized_source)) != 0 ||
        normalize_path(argv[1], cwd, normalized_destination,
                       sizeof(normalized_destination)) != 0 ||
        strcmp(normalized_source, normalized_destination) == 0)
        return -1;

    input = open(argv[0], MINIOS_O_RDONLY);
    if (input < 0)
        return input;

    output = open(argv[1], MINIOS_O_WRONLY | MINIOS_O_CREATE);
    if (output < 0)
    {
        close(input);
        return output;
    }

    result = ftruncate(output, 0);
    while (result == 0)
    {
        int count = read(input, buffer, sizeof(buffer));
        if (count < 0)
        {
            result = count;
            break;
        }
        if (count == 0)
            break;

        int written = 0;
        while (written < count)
        {
            int current = write(output, buffer + written, count - written);
            if (current <= 0)
            {
                result = current < 0 ? current : -1;
                break;
            }
            written += current;
        }
    }

    int output_close_result = close(output);
    int input_close_result = close(input);
    if (result == 0 && output_close_result < 0)
        result = output_close_result;
    if (result == 0 && input_close_result < 0)
        result = input_close_result;
    return result;
}
