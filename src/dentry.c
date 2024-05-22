
#include "dentry.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_dentry.h"
#include "logging.h"

static void dir_ctx_update(struct inode_dir_ctx *ctx, struct ext4_inode *inode, uint32_t lblock) {
    uint64_t dir_pblock = inode_get_data_pblock(inode, lblock, NULL);
    disk_read_block(dir_pblock, ctx->buf);
    ctx->lblock = lblock;
}

struct inode_dir_ctx *dir_ctx_malloc(void) {
    return malloc(sizeof(struct inode_dir_ctx) + BLOCK_SIZE);
}

void dir_ctx_init(struct inode_dir_ctx *ctx, struct ext4_inode *inode) {
    // preload the first logic data block of inode
    dir_ctx_update(ctx, inode, 0);
}

void dir_ctx_free(struct inode_dir_ctx *ctx) {
    free(ctx);
}

// get the dentry from the directory, with a given offset
struct ext4_dir_entry_2 *dentry_next(struct ext4_inode *inode, uint64_t offset, struct inode_dir_ctx *ctx) {
    uint64_t lblock = offset / BLOCK_SIZE;
    uint64_t blk_offset = offset % BLOCK_SIZE;

    uint64_t inode_size = EXT4_INODE_SIZE(inode);
    // DEBUG("offset %lu, lblock %u, blk_offset %u, inode_size %lu", offset, lblock, blk_offset, inode_size);
    ASSERT(inode_size >= offset);
    // if the offset is at the end of the inode, return NULL
    if (inode_size == offset) {
        return NULL;
    }

    if (lblock == ctx->lblock) {
        // if the offset is in the same block, return the dentry
        DEBUG("ctx lblock match lblock %u", lblock);
        return (struct ext4_dir_entry_2 *)&ctx->buf[blk_offset];
    } else {
        DEBUG("ctx lblock %u not match lblock %u, update", ctx->lblock, lblock);
        dir_ctx_update(ctx, inode, lblock);
        return dentry_next(inode, offset, ctx);
    }
}

// find the last dentry
struct ext4_dir_entry_2 *dentry_last(struct ext4_inode *inode, uint64_t *lblock) {
    uint64_t offset = 0;
    struct ext4_dir_entry_2 *de = NULL;
    struct ext4_dir_entry_2 *de_next = NULL;
    struct inode_dir_ctx *dctx = dir_ctx_malloc();
    dir_ctx_init(dctx, inode);
    while ((de_next = dentry_next(inode, offset, dctx))) {
        offset += de_next->rec_len;
        if (de_next->inode_idx == 0 && de_next->name_len == 0) {
            // reach the ext4_dir_entry_tail
            dir_ctx_free(dctx);

            ASSERT(((struct ext4_dir_entry_tail *)de_next)->det_reserved_ft == EXT4_FT_DIR_CSUM);
            INFO("found the last dentry");
            *lblock = offset / BLOCK_SIZE;
            return de;
        }
        de = de_next;
        DEBUG("pass dentry %s[%u]", de_next->name, de_next->rec_len);
    }
    ERR("fail to find the last dentry");
    return NULL;
}

// create a dentry by name and inode_idx
int dentry_create(struct ext4_dir_entry_2 *de, char *name, uint32_t inode_idx) {
    uint16_t left_space = de->rec_len;
    // update de rec_len to be the real rec_len
    de->rec_len = DE_REAL_REC_LEN(de);
    // find the next de starting position
    struct ext4_dir_entry_2 new_de;
    new_de.inode_idx = inode_idx;
    new_de.name_len = strlen(name);
    new_de.rec_len = left_space - de->rec_len;  // left space was set in the new de
    strncpy(new_de.name, name, new_de.name_len);
    new_de.name[new_de.name_len] = 0;
    INFO("create new dentry %s[%u]", new_de.name, inode_idx);
    INFO("left space %u", new_de.rec_len);
    return 0;
}

// add new dentry to the directory
int dentry_add(struct ext4_dir_entry_2 *last_de, struct ext4_dir_entry_2 *de) {
    return 0;
}