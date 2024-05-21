
#include "dentry.h"

#include <stdint.h>
#include <stdlib.h>

#include "disk.h"
#include "ext4/ext4.h"
#include "logging.h"

static void dir_ctx_update(struct inode_dir_ctx *ctx, struct ext4_inode *inode, uint32_t lblock) {
    uint64_t dir_pblock = inode_get_data_pblock(inode, lblock, NULL);
    disk_read_block(dir_pblock, ctx->buf);
    ctx->lblock = lblock;
}

struct inode_dir_ctx *dir_ctx_malloc(void) {
    return malloc(sizeof(struct inode_dir_ctx) + BLOCK_SIZE);
}

// reset the directory context
void dir_ctx_init(struct inode_dir_ctx *ctx, struct ext4_inode *inode) {
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
struct ext4_dir_entry_2 *dentry_last(struct ext4_inode *inode) {
    uint64_t offset = 0;
    struct ext4_dir_entry_2 *de = NULL;
    struct ext4_dir_entry_2 *de_next = NULL;
    struct inode_dir_ctx *dctx = dir_ctx_malloc();
    dir_ctx_init(dctx, inode);
    while ((de_next = dentry_next(inode, offset, dctx))) {
        offset += de_next->rec_len;
        if (!de_next->inode_idx) {
            INFO("found the last dentry");
            dir_ctx_free(dctx);
            // de_next->inode_idx == 0 means its a dummy entry
            // actually de is the last dentry
            return de;
        }
        de = de_next;
        DEBUG("pass dentry %s[%u]", de_next->name, de_next->rec_len);
    }
    ERR("fail to find the last dentry");
    return NULL;
}

// create a dentry by name and inode_idx
int dentry_create(char *name, uint32_t inode_idx, struct ext4_dir_entry_2 *de) {
    return 0;
}

// add new dentry to the directory
int dentry_add(struct ext4_dir_entry_2 *last_de, struct ext4_dir_entry_2 *de) {
    return 0;
}