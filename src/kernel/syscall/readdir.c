#include <syscall.h>

int ksys_readdir(int fd, usize index, dirent_t *out)
{
    if (!out)
        return ERR_INVALID_ARGUMENT;

    fd_table_t *fdt = syscall_get_fd_table();
    struct file *f = fdt ? fd_get(fdt, fd) : NULL;

    if (!f || !f->vn || !f->vn->ops || !f->vn->ops->readdir)
        return ERR_BAD_FILE_DESCRIPTOR;

    const char *name = NULL;
    vnode_type_t type;
    int r = f->vn->ops->readdir(f->vn, index, &name, &type);
    if (r < 0 || !name)
        return ERR_NOT_FOUND;

    strncpy(out->name, name, sizeof(out->name));
    out->name[sizeof(out->name) - 1] = '\0';
    out->type = type;
    out->length = strlen(name);

    return ERR_SUCCESS;
}