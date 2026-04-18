#include "builtin.h"
#include "fs/vfs.h"
#include "libk/log.h"

/* forward declarations of each builtin */
int builtin_cd(int argc, char **argv);
int builtin_pwd(int argc, char **argv);
int builtin_echo(int argc, char **argv);
int builtin_clear(int argc, char **argv);
int builtin_exit(int argc, char **argv);
int builtin_help(int argc, char **argv);
int builtin_history(int argc, char **argv);
int builtin_ls(int argc, char **argv);
int builtin_cat(int argc, char **argv);

static builtin_cmd_t g_builtins[] = {
    {"cd", builtin_cd, "Change directory"},
    {"pwd", builtin_pwd, "Print working directory"},
    {"echo", builtin_echo, "Echo arguments"},
    {"clear", builtin_clear, "Clear the screen"},
    {"exit", builtin_exit, "Exit the shell"},
    {"help", builtin_help, "Show help"},
    {"history", builtin_history, "Show command history"},
    {"ls", builtin_ls, "List directory contents"},
    {"cat", builtin_cat, "Print file contents"},
};

#define BUILTIN_COUNT (sizeof(g_builtins) / sizeof(g_builtins[0]))

const builtin_cmd_t *builtin_lookup(const char *name)
{
    for (usize i = 0; i < BUILTIN_COUNT; ++i)
    {
        if (!strcmp(g_builtins[i].name, name))
            return &g_builtins[i];
    }
    return NULL;
}

static void ensure_bin_dir(void)
{
    struct vnode *root;
    if (vfs_lookup("/", &root) != 0)
        return;

    struct vnode *bin;
    if (vfs_lookup("/bin", &bin) != 0)
    {
        // create /bin
        root->ops->create(root, "bin", VNODE_TYPE_DIR, &bin);
    }
}

void builtin_register_all(void)
{

    ensure_bin_dir();

    vfs_create_exec("/bin/cd", builtin_cd);
    vfs_create_exec("/bin/pwd", builtin_pwd);
    vfs_create_exec("/bin/echo", builtin_echo);
    vfs_create_exec("/bin/clear", builtin_clear);
    vfs_create_exec("/bin/exit", builtin_exit);
    vfs_create_exec("/bin/help", builtin_help);
    vfs_create_exec("/bin/hist", builtin_history);
    vfs_create_exec("/bin/ls", builtin_ls);
    vfs_create_exec("/bin/cat", builtin_cat);
}

const builtin_cmd_t *builtin_iter(usize index)
{
    if (index >= BUILTIN_COUNT)
        return NULL;
    return &g_builtins[index];
}
