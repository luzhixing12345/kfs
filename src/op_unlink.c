
#include <stdlib.h>
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

int unlink_inode(struct ext4_inode *inode, uint32_t inode_idx) {
    // unlink means link_count - 1
    inode->i_links_count--;

    // delete it only if link_count == 0
    if (inode->i_links_count == 0) {
        INFO("delete inode [%d]", inode_idx);
        // just set inode and data bitmap to 0 is ok
        bitmap_inode_set(inode_idx, 0);

        if ((inode->i_mode & S_IFLNK) && EXT4_INODE_GET_SIZE(inode) <= sizeof(inode->i_block)) {
            // if inode is symlink, we don't need to free pblocks
            INFO("unlink a symlink inode %d", inode_idx);
        } else {
            // free all pblocks
            struct pblock_arr p_arr;
            inode_get_all_pblocks(inode, &p_arr);
            bitmap_pblock_free(&p_arr);
        }

        // write back dirty inode
        if (ICACHE_IS_DIRTY(inode)) {
            INFO("write back dirty inode %d", inode_idx);
            icache_write_back((struct icache_entry *)inode);
        }
        ICACHE_SET_INVAL(inode);
    } else {
        ICACHE_SET_DIRTY(inode);
    }
    return 0;
}

int op_unlink(const char *path) {
    DEBUG("unlink %s", path);

    // remove dir inode's dentry
    struct ext4_inode *d_inode;
    uint32_t d_inode_idx;
    if (inode_get_parent_by_path(path, &d_inode, &d_inode_idx) < 0) {
        DEBUG("fail to get dir inode %s", path);
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

    unlink_inode(inode, inode_idx);

    INFO("delete inode %s[%d] decache entry", name, inode_idx);
    decache_delete(path);  // delete from decache

    DEBUG("unlinked inode %d", inode_idx);
    return 0;
}