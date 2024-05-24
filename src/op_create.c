
#include <stdint.h>
#include <string.h>

#include "bitmap.h"
#include "cache.h"
#include "dentry.h"
#include "disk.h"
#include "ext4/ext4.h"
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
    uint64_t parent_lblock;  // for disk write back
    struct ext4_dir_entry_2 *de = dentry_last(inode, parent_idx, &parent_lblock);
    if (de == NULL) {
        ERR("parent inode has no dentry");
        return -ENOENT;
    }
    char *name = strrchr(path, '/') + 1;
    __le16 rec_len = ALIGN_TO_DENTRY(EXT4_DE_BASE_SIZE + strlen(name));
    __le16 left_space = de->rec_len - DE_REAL_REC_LEN(de);
    INFO("for new de %s: require space %u | left space %u", name, rec_len, left_space);
    if (left_space < rec_len) {
        ERR("No space for new dentry");
        return -ENOSPC;
    }
    INFO("space for new dentry is enough");
    // find a valid inode_idx in GDT
    uint32_t inode_idx = bitmap_inode_find(parent_idx);
    if (inode_idx == 0) {
        ERR("No free inode");
        return -ENOSPC;
    }

    // find an empty data block
    uint64_t pblock_idx = bitmap_pblock_find(inode_idx);
    if (pblock_idx == U64_MAX) {
        ERR("No free block");
        return -ENOSPC;
    }

    INFO("new dentry %s: [inode:%u] [block_idx:%u]", name, inode_idx, pblock_idx);

    // create a new dentry, and write back to disk
    dentry_create(de, name, inode_idx);
    ASSERT(dcache->inode_idx == parent_idx && dcache->lblock == parent_lblock);
    disk_write_block(dcache->pblock, dcache->buf);
    INFO("write back new dentry(in parent inode) to disk");

    // for now, inode(parent) is not used any more, reuse it for the new created inode
    // create a new inode
    inode_create(inode_idx, mode, pblock_idx, &inode);
    INFO("create new inode");

    // update inode and data bitmap
    bitmap_inode_set(inode_idx, 1);
    bitmap_pblock_set(pblock_idx, 1);
    // just set bitmap and not write back to disk here for performance
    // write back bitmap to disk when fs is destory
    INFO("set inode and data bitmap");

    // update gdt
    gdt_update(pblock_idx);

    dcache->inode_idx = inode_idx;
    dcache->lblock = 0;
    memset(dcache->buf, 0, BLOCK_SIZE);
    disk_write_block(pblock_idx, dcache->buf);
    INFO("write back new inode to disk");

    return 0;
}