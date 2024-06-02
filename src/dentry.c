
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
struct ext4_dir_entry_2 *dentry_last(struct ext4_inode *inode, uint32_t inode_idx) {
    dcache_init(inode, inode_idx);
    if (ICACHE_CHECK_LAST_DE(inode)) {
        DEBUG("find last dentry offset in icache");
        return ICACHE_GET_LAST_DE(inode);
    }
    uint64_t offset = 0;
    struct ext4_dir_entry_2 *de = NULL;
    struct ext4_dir_entry_2 *de_next = NULL;
    while ((de_next = dentry_next(inode, inode_idx, offset))) {
        if (de_next->inode_idx == 0 && de_next->name_len == 0) {
            // reach the ext4_dir_entry_tail
            ASSERT(((struct ext4_dir_entry_tail *)de_next)->det_reserved_ft == EXT4_FT_DIR_CSUM);
            ICACHE_SET_LAST_DE(inode, de);
            DEBUG("add last dentry %s to icache", de->name);
            return de;
        }
        offset += de_next->rec_len;
        ASSERT(offset <= 4096);
        de = de_next;
        DEBUG("pass dentry %s[%u:%u]", de_next->name, de_next->inode_idx, de_next->rec_len);
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
    uint64_t name_len = strlen(name);
    dcache_init(inode, inode_idx);
    struct ext4_dir_entry_2 *de = NULL, *de_next;
    uint64_t offset = 0;
    while ((de_next = dentry_next(inode, inode_idx, offset))) {
        offset += de_next->rec_len;
        // if reach the end
        if (de_next->inode_idx == 0 && de_next->name_len == 0) {
            ERR("dentry %s not found", name);
            ASSERT(((struct ext4_dir_entry_tail *)de_next)->det_reserved_ft == EXT4_FT_DIR_CSUM);
            return -ENOENT;
        }

        INFO("dentry %s[%d]", de_next->name, de_next->inode_idx);
        if (strncmp(de_next->name, name, name_len) == 0) {
            // delete dentry just means add de_next->rec_len to de->rec_len
            ASSERT(de != NULL);
            INFO("merge de[%s] and de[%s]", de->name, de_next->name);
            INFO("de[%s] rec_len %u -> %u", de->name, de->rec_len, de->rec_len + de_next->rec_len);
            de->rec_len += de_next->rec_len;
            if (ICACHE_GET_LAST_DE(inode) == de_next) {
                // if de_next is the last dentry, set last de to de
                ICACHE_SET_LAST_DE(inode, de);
                DEBUG("set last dentry to %s", de->name);
            }
            return 0;
        }
        de = de_next;
    }
    return -ENOENT;
}

int dentry_has_enough_space(struct ext4_dir_entry_2 *de, const char *name) {
    __le16 rec_len = ALIGN_TO_DENTRY(EXT4_DE_BASE_SIZE + strlen(name));
    __le16 left_space = de->rec_len - DE_REAL_REC_LEN(de);
    INFO("for new de %s: require space %u | left space %u", name, rec_len, left_space);
    if (left_space < rec_len) {
        ERR("No space for new dentry");
        return -ENOSPC;
    }
    INFO("space for new dentry is enough");
    return 0;
}