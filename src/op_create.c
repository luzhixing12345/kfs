
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
    uint32_t parent_idx;
    struct ext4_inode *parent_inode;

    if (inode_get_parent_by_path(path, &parent_inode, &parent_idx) < 0) {
        DEBUG("fail to get parent inode %d", parent_idx);
        return -ENOENT;
    }

    // check if disk has space for a new inode
    uint32_t inode_idx;
    if ((inode_idx = bitmap_inode_find(parent_idx)) == 0) {
        ERR("No space for new inode");
        return -ENOSPC;
    }

    // check if the parent inode has empty space for a new dentry
    struct ext4_dir_entry_2 *de = dentry_last(parent_inode, parent_idx);
    if (de == NULL) {
        ERR("fail to find the last dentry");
        return -ENOENT;
    }

    // create a new dentry, and write back to disk
    char *file_name = strrchr(path, '/') + 1;
    if (dentry_has_enough_space(de, file_name) < 0) {
        // FIXME: try to find a new block for dir
        // TEST-CASE: [012]
        ERR("No space for new dentry");
        return -ENOSPC;
    }

    // create a new dentry in the parent directory
    struct ext4_dir_entry_2 *new_de = dentry_create(de, file_name, inode_idx, EXT4_FT_REG_FILE);
    ICACHE_SET_LAST_DE(parent_inode, new_de);
    dcache_write_back();

    // create a new inode
    struct ext4_inode *inode;
    inode_create(inode_idx, mode, &inode);
    INFO("create new inode");

    // update inode and data bitmap
    bitmap_inode_set(inode_idx, 1);
    // just set bitmap and not write back to disk here for performance
    // write back bitmap to disk when fs is destory
    INFO("set inode and data bitmap");

    // update gdt
    // gdt_update(inode_idx);

    return 0;
}