/*
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

#include "inode.h"

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "dcache.h"
#include "disk.h"
#include "ext4/e4f_dcache.h"
#include "ext4/ext4_super.h"
#include "extents.h"
#include "logging.h"
#include "super.h"

extern struct ext4_super_block sb;
extern struct dcache_entry root;

/* These #defines are only relevant for ext2/3 style block indexing */
#define ADDRESSES_IN_IND_BLOCK  (BLOCK_SIZE / sizeof(uint32_t))
#define ADDRESSES_IN_DIND_BLOCK (ADDRESSES_IN_IND_BLOCK * ADDRESSES_IN_IND_BLOCK)
#define ADDRESSES_IN_TIND_BLOCK (ADDRESSES_IN_DIND_BLOCK * ADDRESSES_IN_IND_BLOCK)
#define MAX_IND_BLOCK           (EXT4_NDIR_BLOCKS + ADDRESSES_IN_IND_BLOCK)
#define MAX_DIND_BLOCK          (MAX_IND_BLOCK + ADDRESSES_IN_DIND_BLOCK)
#define MAX_TIND_BLOCK          (MAX_DIND_BLOCK + ADDRESSES_IN_TIND_BLOCK)

#define ROOT_INODE_N            2  // root inode_idx
#define IS_PATH_SEPARATOR(__c)  ((__c) == '/')

static const char *skip_trailing_backslash(const char *path) {
    while (IS_PATH_SEPARATOR(*path)) path++;
    return path;
}

static uint32_t __inode_get_data_pblock_ind(uint32_t lblock, uint32_t index_block) {
    ASSERT(lblock < ADDRESSES_IN_IND_BLOCK);
    return disk_read_u32(BLOCKS2BYTES(index_block) + lblock * sizeof(uint32_t));
}

static uint32_t __inode_get_data_pblock_dind(uint32_t lblock, uint32_t dindex_block) {
    ASSERT(lblock < ADDRESSES_IN_DIND_BLOCK);

    uint32_t index_block_offset_in_dindex = (lblock / ADDRESSES_IN_IND_BLOCK) * sizeof(uint32_t);
    lblock %= ADDRESSES_IN_IND_BLOCK;

    uint32_t index_block = disk_read_u32(BLOCKS2BYTES(dindex_block) + index_block_offset_in_dindex);

    return __inode_get_data_pblock_ind(lblock, index_block);
}

static uint32_t __inode_get_data_pblock_tind(uint32_t lblock, uint32_t tindex_block) {
    ASSERT(lblock < ADDRESSES_IN_TIND_BLOCK);

    uint32_t dindex_block_offset_in_tindex = (lblock / ADDRESSES_IN_DIND_BLOCK) * sizeof(uint32_t);
    lblock %= ADDRESSES_IN_DIND_BLOCK;

    uint32_t dindex_block = disk_read_u32(BLOCKS2BYTES(tindex_block) + dindex_block_offset_in_tindex);

    return __inode_get_data_pblock_dind(lblock, dindex_block);
}

/* Get pblock for a given inode and lblock.  If extent is not NULL, it will
 * store the length of extent, that is, the number of consecutive pblocks
 * that are also consecutive lblocks (not counting the requested one). */
uint64_t inode_get_data_pblock(struct ext4_inode *inode, uint32_t lblock, uint32_t *extent_len) {
    if (extent_len)
        *extent_len = 1;

    if (inode->i_flags & EXT4_EXTENTS_FL) {
        // inode use ext4 extents
        return extent_get_pblock(&inode->i_block, lblock, extent_len);
    } else {
        // old ext2/3 style, for backward compatibility
        // direct block, indirect block, dindirect block, tindirect block
        ASSERT(lblock <= BYTES2BLOCKS(EXT4_INODE_SIZE(inode)));

        if (lblock < EXT4_NDIR_BLOCKS) {
            return inode->i_block[lblock];
        } else if (lblock < MAX_IND_BLOCK) {
            uint32_t index_block = inode->i_block[EXT4_IND_BLOCK];
            return __inode_get_data_pblock_ind(lblock - EXT4_NDIR_BLOCKS, index_block);
        } else if (lblock < MAX_DIND_BLOCK) {
            uint32_t dindex_block = inode->i_block[EXT4_DIND_BLOCK];
            return __inode_get_data_pblock_dind(lblock - MAX_IND_BLOCK, dindex_block);
        } else if (lblock < MAX_TIND_BLOCK) {
            uint32_t tindex_block = inode->i_block[EXT4_TIND_BLOCK];
            return __inode_get_data_pblock_tind(lblock - MAX_DIND_BLOCK, tindex_block);
        } else {
            /* File-system corruption? */
            ASSERT(0);
        }
    }

    /* Should never reach here */
    ASSERT(0);
    return 0;
}

static void dir_ctx_update(struct inode_dir_ctx *ctx, struct ext4_inode *inode, uint32_t lblock) {
    uint64_t dir_pblock = inode_get_data_pblock(inode, lblock, NULL);
    disk_read_block(dir_pblock, ctx->buf);
    ctx->lblock = lblock;
}

struct inode_dir_ctx *inode_dir_ctx_get(void) {
    return malloc(sizeof(struct inode_dir_ctx) + BLOCK_SIZE);
}

// reset the directory context
void inode_dir_ctx_reset(struct inode_dir_ctx *ctx, struct ext4_inode *inode) {
    dir_ctx_update(ctx, inode, 0);
}

void inode_dir_ctx_put(struct inode_dir_ctx *ctx) {
    free(ctx);
}

// get the dentry from the directory, with a given offset
struct ext4_dir_entry_2 *inode_dentry_get(struct ext4_inode *inode, uint64_t offset, struct inode_dir_ctx *ctx) {
    uint64_t lblock = offset / BLOCK_SIZE;
    uint64_t blk_offset = offset % BLOCK_SIZE;

    uint64_t inode_size = EXT4_INODE_SIZE(inode);
    DEBUG("offset %lu, lblock %u, blk_offset %u, inode_size %lu", offset, lblock, blk_offset, inode_size);
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
        return inode_dentry_get(inode, offset, ctx);
    }
}

int inode_get_by_number(uint32_t n, struct ext4_inode *inode) {
    if (n == 0)
        return -ENOENT;
    n--; /* Inode 0 doesn't exist on disk */

    // read disk by inode number
    uint64_t off = get_inode_offset(n);
    disk_read(off, MIN(EXT4_S_INODE_SIZE(sb), sizeof(struct ext4_inode)), inode);
    return 0;
}

int inode_set_by_number(uint32_t n, struct ext4_inode *inode) {
    if (n == 0)
        return -ENOENT;
    n--; /* Inode 0 doesn't exist on disk */

    // write disk by inode number
    uint64_t off = get_inode_offset(n);
    disk_write(off, MIN(EXT4_S_INODE_SIZE(sb), sizeof(struct ext4_inode)), inode);
    return 0;
}

static uint8_t get_path_token_len(const char *path) {
    uint8_t len = 0;
    while (path[len] != '/' && path[len]) {
        len++;
    }
    return len;
}


struct dcache_entry *get_cached_inode_idx(const char *path) {
    struct dcache_entry *next = NULL;
    struct dcache_entry *ret;

    // /home/kamilu/kfs/test/0001-sanity-small.sh
    // |  1 |   2  | 3 |  4 |        5           |
    do {
        skip_trailing_backslash(path);
        uint8_t path_len = get_path_token_len(path);
        ret = next;

        if (path_len == 0) {
            return ret;
        }

        next = dcache_lookup(ret, path, path_len);
        if (next) {
            path += path_len;
        }
    } while (next != NULL);

    return ret;
}

uint32_t inode_get_idx_by_path(const char *path) {
    struct inode_dir_ctx *dctx = inode_dir_ctx_get();
    uint32_t inode_idx = 0;
    struct ext4_inode inode;

    /* Paths from fuse are always absolute */
    assert(IS_PATH_SEPARATOR(path[0]));

    DEBUG("Looking up: %s", path);

    // first, find in cache
    struct dcache_entry *dc_entry = get_cached_inode_idx(path);
    inode_idx = dc_entry ? dc_entry->inode_idx : root.inode_idx;
    DEBUG("Found inode_idx %d", inode_idx);

    do {
        uint64_t offset = 0;
        struct ext4_dir_entry_2 *de = NULL;

        path = skip_trailing_backslash(path);
        uint8_t path_len = get_path_token_len(path);

        if (path_len == 0) {
            // Reached the end of the path
            break;
        }

        // load inode by inode_idx
        inode_get_by_number(inode_idx, &inode);

        inode_dir_ctx_reset(dctx, &inode);
        while ((de = inode_dentry_get(&inode, offset, dctx))) {
            DEBUG("get dentry %s", de->name);
            offset += de->rec_len;

            // if length not equal or inode is 0, it's not a valid entry, continue
            if (path_len != de->name_len || !de->inode || strncmp(path, de->name, path_len)) {
                DEBUG("not match dentry %s", de->name);
                continue;
            }

            // Found the entry
            INFO("Found entry %s, inode %d", de->name, de->inode);
            inode_idx = de->inode;

            if (S_ISDIR(inode.i_mode)) {
                // if the entry is a directory, add it to the cache
                dc_entry = dcache_insert(dc_entry, path, path_len, inode_idx);
            }
            break;
        }

        /* Couldn't find the entry */
        if (de == NULL) {
            inode_idx = 0;
            break;
        }
    } while ((path = strchr(path, '/')));

    inode_dir_ctx_put(dctx);
    return inode_idx;
}

int inode_get_by_path(const char *path, struct ext4_inode *inode) {
    uint32_t inode_idx = inode_get_idx_by_path(path);
    return inode_get_by_number(inode_idx, inode);
}

// TODO check user permission
int inode_check_permission(struct ext4_inode *inode) {
    // UNIX permission check
    struct fuse_context *cntx = fuse_get_context();
    uid_t uid = cntx->uid;
    gid_t gid = cntx->gid;
    DEBUG("check inode & user permission");
    DEBUG("inode uid %d, inode gid %d", inode->i_uid, inode->i_gid);
    DEBUG("user uid %d, user gid %d", uid, gid);
    // if (inode->i_uid != uid || inode->i_gid != gid) {
    //     INFO("Permission denied");
    //     return -EACCES;
    // }
    INFO("Permission granted");
    return 0;
}

int inode_init(void) {
    return dcache_init_root(ROOT_INODE_N);
}
