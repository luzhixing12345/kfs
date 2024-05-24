
#include "dentry.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cache.h"
#include "ext4/ext4.h"
#include "ext4/ext4_dentry.h"
#include "inode.h"
#include "logging.h"

extern struct dcache *dcache;

// get the dentry from the directory, with a given offset
struct ext4_dir_entry_2 *dentry_next(struct ext4_inode *inode, uint32_t inode_idx, uint64_t offset) {
    uint64_t lblock = offset / BLOCK_SIZE;
    uint64_t blk_offset = offset % BLOCK_SIZE;

    uint64_t inode_size = EXT4_INODE_SIZE(inode);
    // DEBUG("offset %lu, lblock %u, blk_offset %u, inode_size %lu", offset, lblock, blk_offset, inode_size);
    ASSERT(inode_size >= offset);
    // if the offset is at the end of the inode, return NULL
    if (inode_size == offset) {
        return NULL;
    }

    if (lblock == dcache->lblock) {
        // if the offset is in the same block, return the dentry
        DEBUG("dctx lblock match lblock %u", lblock);
        return (struct ext4_dir_entry_2 *)&dcache->buf[blk_offset];
    } else {
        DEBUG("dctx lblock %u not match lblock %u, update", dcache->lblock, lblock);
        dcache_update(inode, lblock);
        return dentry_next(inode, inode_idx, offset);
    }
}

// find the last dentry
struct ext4_dir_entry_2 *dentry_last(struct ext4_inode *inode, uint32_t inode_idx, uint64_t *lblock) {
    uint64_t offset = 0;
    struct ext4_dir_entry_2 *de = NULL;
    struct ext4_dir_entry_2 *de_next = NULL;
    dcache_init(inode, inode_idx);
    while ((de_next = dentry_next(inode, inode_idx, offset))) {
        offset += de_next->rec_len;
        if (de_next->inode_idx == 0 && de_next->name_len == 0) {
            // reach the ext4_dir_entry_tail
            ASSERT(((struct ext4_dir_entry_tail *)de_next)->det_reserved_ft == EXT4_FT_DIR_CSUM);
            *lblock = (offset - 1) / BLOCK_SIZE;
            INFO("reach the last dentry, lblock %u", *lblock);
            return de;
        }
        ASSERT(offset <= 4096);
        de = de_next;
        DEBUG("pass dentry %s[%u:%u]", de_next->name, de_next->inode_idx, de_next->rec_len);
    }
    ERR("fail to find the last dentry");
    return NULL;
}

// create a dentry by name and inode_idx
int dentry_create(struct ext4_dir_entry_2 *de, char *name, uint32_t inode_idx) {
    uint16_t left_space = de->rec_len;
    // update de rec_len to be the real rec_len
    de->rec_len = DE_REAL_REC_LEN(de);
    DEBUG("set dentry %s rec_len to %u", de->name, de->rec_len);
    // find the next de starting position
    struct ext4_dir_entry_2 *new_de = (struct ext4_dir_entry_2 *)((char *)de + de->rec_len);
    new_de->inode_idx = inode_idx;
    new_de->name_len = strlen(name);
    new_de->rec_len = left_space - de->rec_len;  // left space set in the new de
    strncpy(new_de->name, name, new_de->name_len);
    new_de->name[new_de->name_len] = 0;
    INFO("create new dentry %s[%u:%u:%u]", new_de->name, inode_idx, new_de->name_len, new_de->rec_len);
    return 0;
}

int dentry_init(uint32_t parent_idx, uint32_t inode_idx) {
    dcache->inode_idx = inode_idx;
    dcache->lblock = 0;

    // create dentry tail
    struct ext4_dir_entry_tail *de_tail = (struct ext4_dir_entry_tail *)(dcache->buf + BLOCK_SIZE - EXT4_DE_TAIL_SIZE);
    de_tail->det_reserved_zero1 = 0;
    de_tail->det_rec_len = EXT4_DE_TAIL_SIZE;
    de_tail->det_reserved_zero2 = 0;
    de_tail->det_reserved_ft = EXT4_FT_DIR_CSUM;
    de_tail->det_checksum = 0;  // TODO: checksum

    // .
    struct ext4_dir_entry_2 *de_dot = (struct ext4_dir_entry_2 *)(dcache->buf);
    de_dot->inode_idx = inode_idx;
    de_dot->rec_len = EXT4_DE_DOT_SIZE;
    de_dot->name_len = 1;
    de_dot->file_type = EXT4_FT_DIR;
    strncpy(de_dot->name, ".", 1);
    de_dot->name[1] = 0;

    // ..
    struct ext4_dir_entry_2 *de_dot2 = (struct ext4_dir_entry_2 *)(dcache->buf + EXT4_DE_DOT_SIZE);
    de_dot2->inode_idx = parent_idx;
    de_dot2->rec_len = BLOCK_SIZE - EXT4_DE_DOT_SIZE - EXT4_DE_TAIL_SIZE;
    de_dot2->name_len = 2;
    de_dot2->file_type = EXT4_FT_DIR;
    strncpy(de_dot2->name, "..", 2);
    de_dot2->name[2] = 0;
    return 0;
}