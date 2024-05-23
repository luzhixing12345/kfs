/*
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "cache.h"
#include "dentry.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_inode.h"
#include "extents.h"
#include "logging.h"

extern struct ext4_super_block sb;
extern struct dcache_entry root;
extern struct ext4_group_desc *gdt;
extern struct dcache *dcache;

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
    if (extent_len) {
        *extent_len = 1;
    }

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

int inode_get_by_number(uint32_t inode_idx, struct ext4_inode *inode) {
    if (inode_idx == 0) {
        return -ENOENT;
    }
    inode_idx--; /* Inode 0 doesn't exist on disk */

    // first try to find in icache
    struct ext4_inode *ic_entry = icache_find(inode_idx);
    if (ic_entry) {
        DEBUG("Found inode_idx %d in icache", inode_idx);
        memcpy(inode, ic_entry, sizeof(struct ext4_inode));
        return 0;
    }

    // read disk by inode number
    uint64_t off = inode_get_offset(inode_idx);
    disk_read(off, MIN(EXT4_S_INODE_SIZE(sb), sizeof(struct ext4_inode)), inode);

    // add to icache
    DEBUG("Add inode_idx %d to icache", inode_idx);
    icache_insert(inode_idx, inode);
    return 0;
}

static uint8_t get_path_token_len(const char *path) {
    uint8_t len = 0;
    while (path[len] != '/' && path[len]) {
        len++;
    }
    return len;
}

struct dcache_entry *get_cached_inode_idx(const char **path) {
    struct dcache_entry *next = NULL;
    struct dcache_entry *ret;

    do {
        *path = skip_trailing_backslash(*path);
        uint8_t path_len = get_path_token_len(*path);
        ret = next;

        if (path_len == 0) {
            return ret;
        }

        next = dcache_lookup(ret, *path, path_len);
        if (next) {
            INFO("Found entry in cache: %s", next->name);
            *path += path_len;
        }
    } while (next != NULL);

    return ret;
}

uint32_t inode_get_idx_by_path(const char *path) {
    uint32_t inode_idx = 0;
    struct ext4_inode inode;

    /* Paths from fuse are always absolute */
    assert(IS_PATH_SEPARATOR(path[0]));

    DEBUG("Looking up: %s", path);

    // first try to find in dcache
    struct dcache_entry *dc_entry = get_cached_inode_idx(&path);
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
        dcache_init(&inode, inode_idx);
        while ((de = dentry_next(&inode, offset))) {
            offset += de->rec_len;
            if (!de->inode_idx) {
                INFO("reach the end of the directory");
                continue;
            }
            INFO("get dentry %s[%d]", de->name, de->inode_idx);

            // if length not equal, continue
            if (path_len != de->name_len || strncmp(path, de->name, path_len)) {
                INFO("not match dentry %s", de->name);
                continue;
            }

            // Found the entry
            INFO("Found entry %s, inode %d", de->name, de->inode_idx);
            inode_idx = de->inode_idx;

            // if the entry is a directory, add it to the cache
            if (de->file_type == EXT4_FT_DIR) {
                INFO("Add dir entry %s:%d to cache", path, path_len);
                dc_entry = dcache_insert(dc_entry, path, path_len, inode_idx);
            }
            break;
        }

        /* Couldn't find the entry */
        if (de == NULL) {
            inode_idx = 0;
            INFO("Couldn't find entry %s", path);
            break;
        }
    } while ((path = strchr(path, '/')));

    return inode_idx;
}

int inode_get_by_path(const char *path, struct ext4_inode *inode, uint32_t *inode_idx) {
    uint32_t i_idx = inode_get_idx_by_path(path);
    if (i_idx == 0) {
        return -ENOENT;
    }
    if (inode_idx != NULL) {
        *inode_idx = i_idx;
    }
    return inode_get_by_number(i_idx, inode);
}

uint64_t inode_get_offset(uint32_t inode_idx) {
    // first calculate inode_idx in which group
    uint32_t group_idx = inode_idx / EXT4_INODES_PER_GROUP(sb);
    ASSERT(group_idx < EXT4_N_BLOCK_GROUPS(sb));

    // get inode table offset by gdt
    uint64_t off = EXT4_DESC_INODE_TABLE(gdt[group_idx]);
    off = BLOCKS2BYTES(off) + (inode_idx % EXT4_INODES_PER_GROUP(sb)) * EXT4_S_INODE_SIZE(sb);
    DEBUG("inode_idx: %u, group_idx: %u, off: %lu", inode_idx, group_idx, off);
    return off;
}

int inode_check_permission(struct ext4_inode *inode, access_mode_t mode) {
    // UNIX permission check
    struct fuse_context *cntx = fuse_get_context();  //获取当前用户的上下文
    uid_t uid = cntx->uid;
    gid_t gid = cntx->gid;
    DEBUG("check inode & user permission on op mode %d", mode);
    DEBUG("inode uid %d, inode gid %d", inode->i_uid, inode->i_gid);
    DEBUG("user uid %d, user gid %d", uid, gid);

    // 检查用户是否是超级用户
    if (uid == 0) {
        INFO("Permission granted");
        return 0;  // root用户允许所有操作
    }

    // 根据访问模式、文件所有者检查权限(只读、只写、读写)
    if (inode->i_uid == uid) {
        if ((mode == RD_ONLY && (inode->i_mode & S_IRUSR)) || (mode == WR_ONLY && (inode->i_mode & S_IWUSR)) ||
            (mode == RDWR && ((inode->i_mode & S_IRUSR) && (inode->i_mode & S_IWUSR)))) {
            INFO("Permission granted");
            return 0;  // 允许操作
        }
    }

    // 组用户检查
    if (inode->i_gid == gid) {
        if ((mode == RD_ONLY && (inode->i_mode & S_IRGRP)) || (mode == WR_ONLY && (inode->i_mode & S_IWGRP)) ||
            (mode == RDWR && ((inode->i_mode & S_IRGRP) && (inode->i_mode & S_IWGRP)))) {
            INFO("Permission granted");
            return 0;  // 允许操作
        }
    }

    return -EACCES;  // 访问拒绝
}

// find the path's directory inode_idx
uint32_t inode_get_parent_idx_by_path(const char *path) {
    char *tmp = strdup(path);
    char *last_slashp = strrchr(tmp, '/');
    if (last_slashp == path) {
        return 0;
    }
    *(last_slashp + 1) = '\0';
    uint32_t parent_idx = inode_get_idx_by_path(tmp);
    free(tmp);
    return parent_idx;
}