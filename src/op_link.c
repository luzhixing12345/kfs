
#include <string.h>

#include "bitmap.h"
#include "cache.h"
#include "dentry.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_basic.h"
#include "ext4/ext4_inode.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

int op_link(const char *from, const char *to) {
    DEBUG("hard link from %s to %s", from, to);

    struct ext4_inode *inode;
    uint32_t inode_idx;
    if (inode_get_by_path(from, &inode, &inode_idx) < 0) {
        DEBUG("fail to get inode %s", to);
        return -ENOENT;
    }

    if (S_ISDIR(inode->i_mode)) {
        ERR("hard link is not supported for directory");
        return -EPERM;
    }

    // hard link
    if (inode->i_links_count != UINT16_MAX) {
        inode->i_links_count++;
        ICACHE_SET_DIRTY(inode);
        DEBUG("inode[%u] links %u", inode_idx, inode->i_links_count);
    } else {
        ERR("inode[%u] links count overflow", inode_idx);
        return -EMLINK;
    }

    // create a new dentry
    struct ext4_inode *to_dir_inode;
    uint32_t to_dir_inode_idx;
    if (inode_get_parent_by_path(to, &to_dir_inode, &to_dir_inode_idx) < 0) {
        DEBUG("fail to get inode %s", to);
        return -ENOENT;
    }

    struct ext4_dir_entry_2 *last_de = dentry_last(to_dir_inode, to_dir_inode_idx);
    char *name = strrchr(to, '/') + 1;
    if (dentry_has_enough_space(last_de, name) < 0) {
        ERR("No space for new dentry");
        return -ENOSPC;
    }
    struct ext4_dir_entry_2 *new_de = dentry_create(last_de, name, inode_idx, inode_mode2type(inode->i_mode));
    dcache_write_back();
    ICACHE_SET_LAST_DE(to_dir_inode, new_de);
    return 0;
}