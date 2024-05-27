
#include <string.h>
#include "bitmap.h"
#include "cache.h"
#include "dentry.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

int op_rmdir(const char *path) {
    DEBUG("rmdir %s", path);

    // remove parent inode's dentry

    struct ext4_inode *inode;
    uint32_t inode_idx;
    if (inode_get_by_path(path, &inode, &inode_idx) < 0) {
        DEBUG("fail to get inode %s", path);
        return -ENOENT;
    }
    if (!S_ISDIR(inode->i_mode)) {
        return -EISDIR;
    }

    // unlink means link_count - 1
    inode->i_links_count--;

    // delete it only if link_count == 0
    if (inode->i_links_count == 0) {
        INFO("delete inode %d", inode_idx);
        // just set inode and data bitmap to 0 is ok
        bitmap_inode_set(inode_idx, 0);
        struct pblock_arr p_arr;
        inode_get_all_pblocks(inode, &p_arr);
        bitmap_pblock_free(&p_arr);
        ICACHE_INVAL(inode);
    } else {
        ICACHE_DIRTY(inode);
    }

    // remove dir inode's dentry
    struct ext4_inode *d_inode;
    uint32_t d_inode_idx;
    if (inode_get_parent_by_path(path, &d_inode, &d_inode_idx) < 0) {
        DEBUG("fail to get dir inode %s", path);
        return -ENOENT;
    }
    if (dentry_delete(d_inode, d_inode_idx, strrchr(path, '/') + 1) < 0) {
        ERR("fail to delete dentry %s", strrchr(path, '/') + 1);
        return -ENOENT;
    }
    dcache_write_back();
    decache_delete(path);

    DEBUG("rmdir %s done", path);
    return 0;
}