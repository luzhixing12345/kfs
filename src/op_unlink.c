
#include <string.h>

#include "bitmap.h"
#include "cache.h"
#include "dentry.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_inode.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

int op_unlink(const char *path) {
    DEBUG("unlink %s", path);

    // remove dir inode's dentry
    struct ext4_inode *d_inode;
    uint32_t d_inode_idx = inode_get_parent_idx_by_path(path);
    if (d_inode_idx == 0) {
        DEBUG("fail to get inode %s", path);
        return -ENOENT;
    }
    if (inode_get_by_number(d_inode_idx, &d_inode) < 0) {
        DEBUG("fail to get inode %d", d_inode_idx);
        return -ENOENT;
    }
    DEBUG("get dir inode %d", d_inode_idx);

    struct ext4_inode *inode;
    uint32_t inode_idx;
    if (inode_get_by_path(path, &inode, &inode_idx) < 0) {
        DEBUG("fail to get inode %s", path);
        return -ENOENT;
    }
    DEBUG("get inode %d", inode_idx);

    char *name = strrchr(path, '/') + 1;
    if (dentry_delete(d_inode, d_inode_idx, name) < 0) {
        ERR("fail to delete dentry %s", name);
        return -ENOENT;
    }
    dcache_write_back();

    // unlink means link_count - 1
    inode->i_links_count--;

    // delete it only if link_count == 0
    if (inode->i_links_count == 0) {
        // just set inode and data bitmap to 0 is ok
        bitmap_inode_set(inode_idx, 0);
    } else {
        ICACHE_DIRTY(inode);
    }

    return 0;
}