
#include <string.h>

#include "bitmap.h"
#include "cache.h"
#include "dentry.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_dentry.h"
#include "ext4/ext4_inode.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

// TODO: check if to exsists
// TODO: ln -s xx dir
int op_symlink(const char *from, const char *to) {
    DEBUG("soft symlink from %s to %s", from, to);
    struct ext4_inode *inode;
    uint32_t inode_idx;
    if (inode_get_by_path(from, &inode, &inode_idx) < 0) {
        DEBUG("fail to get inode %s", to);
        return -ENOENT;
    }
    // symbolic link do not increase link count!

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
    dentry_create(last_de, name, inode_idx, EXT4_FT_SYMLINK);
    dcache_write_back();
    return 0;
}