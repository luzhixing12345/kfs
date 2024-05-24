
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
    uint64_t parent_lblock;  // for disk write back
    struct ext4_dir_entry_2 *de = dentry_last(inode, parent_idx, &parent_lblock);
    if (de == NULL) {
        ERR("parent inode has no dentry");
        return -ENOENT;
    }
    char *dir_name = strrchr(path, '/') + 1;
    __le16 rec_len = ALIGN_TO_DENTRY(EXT4_DE_BASE_SIZE + strlen(dir_name));
    __le16 left_space = de->rec_len - DE_REAL_REC_LEN(de);
    INFO("for new de %s: require space %u | left space %u", dir_name, rec_len, left_space);
    if (left_space < rec_len) {
        ERR("No space for new dentry");
        return -ENOSPC;
    }
    INFO("space for new dentry is enough");
    // find a valid inode_idx in GDT
    uint32_t dir_idx = bitmap_inode_find(parent_idx);
    if (dir_idx == 0) {
        ERR("No free inode");
        return -ENOSPC;
    }

    // find an empty data block
    uint64_t dir_pblock_idx = bitmap_pblock_find(dir_idx);
    if (dir_pblock_idx == U64_MAX) {
        ERR("No free block");
        return -ENOSPC;
    }

    INFO("new dentry %s: [inode:%u] [block_idx:%u]", dir_name, dir_idx, dir_pblock_idx);

    // create a new dentry, and write back to disk
    dentry_create(de, dir_name, dir_idx);
    ASSERT(dcache->inode_idx == parent_idx && dcache->lblock == parent_lblock);
    disk_write_block(dcache->pblock, dcache->buf);
    INFO("write back new dentry(in parent inode) to disk");

    // set parent inode link count + 1 because ..
    inode->i_links_count++;
    ICACHE_DIRTY(inode);

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

    // add . and .. for the new dentry
    dentry_init(parent_idx, dir_idx);
    INFO("add . and .. for the new dentry");
    disk_write_block(dir_pblock_idx, dcache->buf);
    INFO("write back . and .. for the new dentry to disk");
    return 0;
}