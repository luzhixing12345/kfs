
#include <errno.h>

#include "logging.h"
#include "ops.h"

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 */
int op_access(const char *path, int mask) {
    DEBUG("access path %s with mask %o", path, mask);

    if (!(mask & X_OK)) {
        ERR("Permission denied");
        return -EACCES;
    }

    struct stat stbuf;
    int err = 0;
    err = op_getattr(path, &stbuf, NULL);
    if (!err) {
        if (S_ISREG(stbuf.st_mode) && !(stbuf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
            INFO("Permission denied");
            err = -EACCES;
        } else if (S_ISDIR(stbuf.st_mode)) {
            // TODO: if path is a directory, preload dentry inode
        }
    }
    INFO("Permission granted");
    return err;
}