
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "bitmap.h"
#include "cache.h"
#include "dentry.h"
#include "disk.h"
#include "ext4/ext4_basic.h"
#include "ext4/ext4_super.h"
#include "logging.h"
#include "ops.h"

extern struct dcache *dcache;

/** Create a directory
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits use  mode|S_IFDIR
 * */
int op_mkdir(const char *path, mode_t mode) {
    mode = mode | S_IFDIR;
    DEBUG("mkdir %s with mode %o", path, mode);

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
    uint64_t lblock;  // for disk write back
    struct ext4_dir_entry_2 *de = dentry_last(inode, parent_idx, &lblock);
    if (de == NULL) {
        ERR("parent inode has no dentry");
        return -ENOENT;
    }
    char *dir_name = strrchr(path, '/') + 1;
    __le16 rec_len = ALIGN_TO_DENTRY(EXT4_DE_BASE_SIZE + strlen(dir_name));
    __le16 left_space = de->rec_len - DE_REAL_REC_LEN(de);
    INFO("dir_name %s, rec_len %u, last de left space %u", dir_name, rec_len, left_space);
    if (left_space < rec_len) {
        ERR("No space for new dentry");
        return -ENOSPC;
    }
    // find a valid inode_idx in GDT
    uint32_t dir_idx = bitmap_inode_find(parent_idx);
    if (dir_idx == 0) {
        ERR("No free inode");
        return -ENOSPC;
    }

    // find an empty data block
    uint64_t pblock_idx = bitmap_pblock_find(dir_idx);
    if (pblock_idx == 0) {
        ERR("No free block");
        return -ENOSPC;
    }

    INFO("new dentry %s, dir_idx %u, block_idx %u", dir_name, dir_idx, pblock_idx);
    INFO("start to create new dentry and new inode");

    // create a new dentry
    dentry_create(de, dir_name, dir_idx);

    // write back new dentry to disk
    ASSERT(dcache->lblock == lblock);
    uint64_t pblock = inode_get_data_pblock(inode, lblock, NULL);
    disk_write_block(pblock, &dcache->buf);
    INFO("write back new dentry to disk");

    // set parent inode link count + 1 because ..
    inode->i_links_count++;
    ICACHE_DIRTY(inode);

    // for now, inode(parent) is not used any more, reuse for new directory inode

    // create a new inode
    inode_create(dir_idx, mode, pblock_idx, &inode);
    INFO("create new inode");

    // update inode and data bitmap
    bitmap_inode_set(parent_idx, 1);
    bitmap_pblock_set(dir_idx, 1);
    // just set bitmap and not write back to disk here for performance
    // write back bitmap to disk when fs is destory
    INFO("set inode and data bitmap");

    // update gdt
    int group_idx = dir_idx / EXT4_INODES_PER_GROUP(sb);
    uint32_t free_blocks = EXT4_GDT_FREE_BLOCKS(&gdt[group_idx]) - 1;
    gdt[group_idx].bg_free_blocks_count_lo = free_blocks & MASK_16;
    gdt[group_idx].bg_free_blocks_count_hi = free_blocks >> 16;

    uint32_t free_inodes = EXT4_GDT_FREE_INODES(&gdt[group_idx]) - 1;
    gdt[group_idx].bg_free_inodes_count_lo = free_inodes & MASK_16;
    gdt[group_idx].bg_free_inodes_count_hi = free_inodes >> 16;

    // used_dirs may overflow
    uint32_t used_dirs = EXT4_GDT_USED_DIRS(&gdt[group_idx]);
    if (used_dirs != EXT4_GDT_MAX_USED_DIRS) {
        ERR("used dirs overflow!");
        used_dirs++;
    }
    gdt[group_idx].bg_used_dirs_count_lo = used_dirs & MASK_16;
    gdt[group_idx].bg_used_dirs_count_hi = used_dirs >> 16;

    uint32_t unused_inodes = EXT4_GDT_ITABLE_UNUSED(&gdt[group_idx]) - 1;
    gdt[group_idx].bg_itable_unused_lo = unused_inodes & MASK_16;
    gdt[group_idx].bg_itable_unused_hi = unused_inodes >> 16;
    EXT4_GDT_SET_DIRTY(&gdt[group_idx]);  // set gdt dirty
    INFO("update gdt");

    // add . and .. for the new dentry
    // dentry_create(de, ".", dir_idx);
    // dentry_create(de, "..", parent_idx);
    // INFO("add . and .. for the new dentry");
    return 0;
}