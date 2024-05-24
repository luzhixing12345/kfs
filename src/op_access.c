
#include <errno.h>
#include <sys/types.h>
#include "inode.h"
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

    struct ext4_inode *inode;
    if (inode_get_by_path(path, &inode, NULL) < 0) {
        DEBUG("fail to get inode %s", path);
        return -ENOENT;
    }

    // check permission
    struct fuse_context *cntx = fuse_get_context();
    uid_t uid = cntx->uid;
    gid_t gid = cntx->gid;

    // root user allow all permission
    if (uid == 0) {
        INFO("Permission granted");
        return 0;
    }

    if (uid == EXT4_INODE_UID(*inode)) {
        if (inode->i_mode & S_IXUSR) {
            INFO("Permission granted");
            return 0;
        }
    }

    if (gid == EXT4_INODE_GID(*inode)) {
        if (inode->i_mode & S_IXGRP) {
            INFO("Permission granted");
            return 0;
        }
    } else {
        if (inode->i_mode & S_IXOTH) {
            INFO("Permission granted");
            return 0;  
        }
    }

    // no permission
    ERR("Permission denied");
    return -EACCES;
}