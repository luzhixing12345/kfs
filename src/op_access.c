
#include <errno.h>
#include <sys/types.h>

#include "ext4/ext4_inode.h"
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

    struct ext4_inode *inode;

    if (inode_get_by_path(path, &inode, NULL) < 0) {
        DEBUG("fail to get inode %s", path);
        return -ENOENT;
    }

    // check permission
    if (mask & W_OK) {
        if (inode_check_permission(inode, WRITE) < 0) {
            ERR("Permission denied");
            return -EACCES;
        }
    }

    if (mask & R_OK) {
        if (inode_check_permission(inode, READ) < 0) {
            ERR("Permission denied");
            return -EACCES;
        }
    }

    if (mask & X_OK) {
        if (inode_check_permission(inode, EXEC) < 0) {
            ERR("Permission denied");
            return -EACCES;
        }
    }

    return 0;
}