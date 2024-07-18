
#include <stdint.h>
#include <string.h>

#include "bitmap.h"
#include "cache.h"
#include "dentry.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_dentry.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

extern struct dcache *dcache;

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 */
int op_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    // the only difference between create and mkdir is that
    //
    // - mkdir need dentry_init() for . and ..
    // - mkdir need parent inode.i_links_count++

    DEBUG("create path %s with mode %o", path, mode);

    // first find the parent directory of this path
    uint32_t parent_idx = inode_get_parent_idx_by_path(path);
    if (parent_idx == 0) {
        // root directory
        return -ENOENT;
    }
    struct ext4_inode *inode;
    if (inode_get_by_number(parent_idx, &inode) < 0) {
        DEBUG("fail to get parent inode %d", parent_idx);
        return -ENOENT;
    }

    // check if the parent inode has empty space for a new dentry
    struct ext4_dir_entry_2 *de = dentry_last(inode, parent_idx);
    if (de == NULL) {
        ERR("parent inode has no dentry");
        return -ENOENT;
    }
    // check if disk has space for a new inode
    uint32_t dir_idx;
    uint64_t dir_pblock_idx;
    if (bitmap_find_space(parent_idx, &dir_idx, &dir_pblock_idx) < 0) {
        ERR("No space for new inode");
        return -ENOSPC;
    }

    INFO("check ok!");
    // create a new dentry, and write back to disk
    char *dir_name = strrchr(path, '/') + 1;
    if (dentry_has_enough_space(de, dir_name) < 0) {
        ERR("No space for new dentry");
        return -ENOSPC;
    }

    struct ext4_dir_entry_2 *new_de = dentry_create(de, dir_name, dir_idx, EXT4_FT_REG_FILE);
    ICACHE_SET_LAST_DE(inode, new_de);
    dcache_write_back();

    // for now, inode(parent) is not used any more, reuse it for the new created inode
    // create a new inode
    inode_create(dir_idx, mode, dir_pblock_idx, &inode);
    INFO("create new inode");

    // update inode and data bitmap
    bitmap_inode_set(dir_idx, 1);
    bitmap_pblock_set(dir_pblock_idx, 1);
    // just set bitmap and not write back to disk here for performance
    // write back bitmap to disk when fs is destory
    INFO("set inode and data bitmap");

    // update gdt
    gdt_update(dir_idx);

    return 0;
}