
#include "dentry.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cache.h"
#include "ext4/ext4.h"
#include "ext4/ext4_dentry.h"
#include "ext4/ext4_inode.h"
#include "inode.h"
#include "logging.h"

extern struct dcache *dcache;

// get the dentry from the directory, with a given offset
struct ext4_dir_entry_2 *dentry_next(struct ext4_inode *inode, uint32_t inode_idx, uint64_t offset) {
    uint64_t lblock = offset / BLOCK_SIZE;
    uint64_t blk_offset = offset % BLOCK_SIZE;

    uint64_t inode_size = EXT4_INODE_GET_SIZE(inode);
    // DEBUG("offset %lu, lblock %u, blk_offset %u, inode_size %lu", offset, lblock, blk_offset, inode_size);
    ASSERT(inode_size >= offset);
    // if the offset is at the end of the inode, return NULL
    if (inode_size == offset) {
        DEBUG("reach block boundary at offset %lu", offset);
        if (EXT4_INODE_GET_BLOCKS(inode) * BLOCK_SIZE > inode_size) {
            EXT4_INODE_SET_SIZE(inode, inode_size + BLOCK_SIZE);
            DEBUG("increase inode size to %lu", inode_size + BLOCK_SIZE);
        }
        DEBUG("inode space not enough")
        return NULL;
    }

    if (lblock == dcache->lblock) {
        // if the offset is in the same block, return the dentry
        DEBUG("dctx lblock match lblock %u", lblock);
        return (struct ext4_dir_entry_2 *)&dcache->buf[blk_offset];
    } else {
        DEBUG("dctx lblock %u not match lblock %u, update", dcache->lblock, lblock);
        dcache_load_lblock(inode, lblock);
        return dentry_next(inode, inode_idx, offset);
    }
}

// find the last dentry
struct ext4_dir_entry_2 *dentry_last(struct ext4_inode *inode, uint32_t inode_idx) {
    dcache_init(inode, inode_idx);
    if (ICACHE_CHECK_LAST_DE(inode)) {
        DEBUG("find last dentry offset in icache");
        return ICACHE_GET_LAST_DE(inode);
    }

    struct ext4_dir_entry_2 *de = dentry_find(inode, inode_idx, NULL, NULL);
    if (de != NULL) {
        return de;
    }
    ERR("fail to find the last dentry");
    return NULL;
}

// create a dentry by name and inode_idx
struct ext4_dir_entry_2 *dentry_create(struct ext4_dir_entry_2 *de, char *name, uint32_t inode_idx, int file_type) {
    uint16_t left_space = de->rec_len;
    // update de rec_len to be the real rec_len
    de->rec_len = DE_REAL_REC_LEN(de);
    DEBUG("set dentry %s rec_len to %u", de->name, de->rec_len);
    // find the next de starting position
    struct ext4_dir_entry_2 *new_de = (struct ext4_dir_entry_2 *)((char *)de + de->rec_len);
    new_de->inode_idx = inode_idx;
    new_de->name_len = strlen(name);
    new_de->rec_len = left_space - de->rec_len;  // left space set in the new de
    new_de->file_type = file_type;
    strncpy(new_de->name, name, new_de->name_len);
    new_de->name[new_de->name_len] = 0;
    INFO("create new dentry %s[%u:%u:%u]", new_de->name, inode_idx, new_de->name_len, new_de->rec_len);

    return new_de;
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

int dentry_delete(struct ext4_inode *inode, uint32_t inode_idx, char *name) {
    INFO("try to delete dentry %s", name);
    dcache_init(inode, inode_idx);
    struct ext4_dir_entry_2 *de, *de_before;
    de = dentry_find(inode, inode_idx, name, &de_before);
    ASSERT(de != NULL);

    // delete dentry just means add de_next->rec_len to de->rec_len
    INFO("merge de[%s] and de[%s]", de_before->name, de->name);
    INFO("de[%s] rec_len %u -> %u", de_before->name, de_before->rec_len, de_before->rec_len + de->rec_len);
    de_before->rec_len += de->rec_len;
    if (ICACHE_GET_LAST_DE(inode) == de) {
        // if de_next is the last dentry, set last de to de
        ICACHE_SET_LAST_DE(inode, de_before);
        DEBUG("set last dentry to %s", de_before->name);
    }
    return 0;
}

/**
 * @brief
 *
 * @param de
 * @param name_len
 * @return int
 * -ENOSPC: no space
 */
int dentry_has_enough_space(struct ext4_dir_entry_2 *de, uint64_t name_len) {
    __le16 rec_len = DE_CALC_REC_LEN(name_len);
    __le16 left_space = de->rec_len - DE_REAL_REC_LEN(de);
    INFO("require space %u | left space %u", rec_len, left_space);
    if (left_space < rec_len) {
        // TODO: +sizeof(tail) check if len is enough
        WARNING("No space for new dentry in de[%s]", de->name);
        return -ENOSPC;
    }
    INFO("space for new dentry is enough");
    return 0;
}

/**
 * @brief find dentry by name
 *
 * @param inode
 * @param inode_idx
 * @param name dentry name; find the last dentry if name is NULL
 * @param de_before if not NULL, set the dentry before the found one
 * @return struct ext4_dir_entry_2*
 */
struct ext4_dir_entry_2 *dentry_find(struct ext4_inode *inode, uint32_t inode_idx, char *name,
                                     struct ext4_dir_entry_2 **de_before) {
    struct ext4_dir_entry_2 *de = NULL;
    struct ext4_dir_entry_2 *de_next = NULL;
    dcache_init(inode, inode_idx);
    uint64_t offset = 0;
    uint64_t name_len = name ? strlen(name) : 0;
    while ((de_next = dentry_next(inode, inode_idx, offset))) {
        if (de_next->inode_idx == 0 && de_next->name_len == 0) {
            // reach the ext4_dir_entry_tail
            ASSERT(((struct ext4_dir_entry_tail *)de_next)->det_reserved_ft == EXT4_FT_DIR_CSUM);
            if (name == NULL) {
                // name is NULL means find the last dentry
                ICACHE_SET_LAST_DE(inode, de);
                DEBUG("add last dentry %s to icache", de->name);
                return de;
            } else {
                DEBUG("fail to find dentry %s", name);
                return NULL;
            }
        }
        DEBUG("pass dentry %s[%u:%u]", de_next->name, de_next->inode_idx, de_next->rec_len);
        offset += de_next->rec_len;
        if (name_len == de_next->name_len && strncmp(de_next->name, name, name_len) == 0) {
            if (de_before != NULL) {
                *de_before = de;
            }
            INFO("find dentry %s", name);
            return de_next;
        }
        de = de_next;
    }
    ERR("fail to find the last dentry");
    return NULL;
}