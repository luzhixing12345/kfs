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
#include <sys/types.h>
#include <time.h>

#include "bitmap.h"
#include "cache.h"
#include "common.h"
#include "ctl.h"
#include "dentry.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_extents.h"
#include "ext4/ext4_inode.h"
#include "extents.h"
#include "fuse.h"
#include "logging.h"

extern struct ext4_super_block sb;
extern struct decache_entry *root;
extern struct ext4_group_desc *gdt;
extern struct dcache *dcache;

const char *skip_trailing_backslash(const char *path) {
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

/**
 * @brief Get pblock for a given inode and lblock.  If extent is not NULL, it will
 * store the length of extent, that is, the number of consecutive pblocks
 * that are also consecutive lblocks (not counting the requested one).
 *
 * @param inode
 * @param lblock
 * @param extent_len
 * @return uint64_t
 */
uint64_t inode_get_data_pblock(struct ext4_inode *inode, uint32_t lblock, uint32_t *extent_len) {
    if (inode->i_flags & EXT4_EXTENTS_FL) {
        // inode use ext4 extents
        return extent_get_pblock(&inode->i_block, lblock, extent_len);
    } else {
        // old ext2/3 style, for backward compatibility
        // direct block, indirect block, dindirect block, tindirect block
        ASSERT(lblock <= BYTES2BLOCKS(EXT4_INODE_GET_SIZE(inode)));

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

// TODO: ext4 extents data pblock
int inode_get_all_pblocks(struct ext4_inode *inode, struct pblock_arr *pblock_arr) {
    // uint64_t inode_block_count = EXT4_INODE_GET_SIZE(inode);
    if (inode->i_flags & EXT4_EXTENTS_FL) {
        // inode use ext4 extents
        struct ext4_extent_header *eh = (struct ext4_extent_header *)&inode->i_block;
        struct ext4_extent *ee = (struct ext4_extent *)(eh + 1);

        if (eh->eh_depth == 0) {
            pblock_arr->len = eh->eh_entries;
            if (pblock_arr->len == 0) {
                // short symbolic link has no pblocks
                return 0;
            }
            ASSERT(eh->eh_magic == EXT4_EXT_MAGIC);
            pblock_arr->arr = malloc(pblock_arr->len * sizeof(struct pblock_range));
            for (int i = 0; i < pblock_arr->len; i++) {
                pblock_arr->arr[i].pblock = EXT4_EXT_GET_PADDR(ee[i]);
                pblock_arr->arr[i].len = ee[i].ee_len;
            }
            INFO("get all pblocks done");
        } else {
            // FIXME: recusively get all pblocks when eh->eh_depth > 0
            ASSERT(0);
        }
    } else {
        // old ext2/3 style, for backward compatibility
        // direct block, indirect block, dindirect block, tindirect block
        ASSERT(0);
    }
    return 0;
}

int inode_get_by_number(uint32_t inode_idx, struct ext4_inode **inode) {
    if (inode_idx == 0) {
        return -ENOENT;
    }

    // first try to find in icache
    struct ext4_inode *ic_entry = icache_find(inode_idx);
    if (ic_entry) {
        DEBUG("Found inode_idx %d in icache", inode_idx);
        *inode = ic_entry;
    } else {
        // inode could not find in icache, read from disk and return the inode
        DEBUG("Not found inode_idx %d in icache", inode_idx);
        *inode = icache_insert(inode_idx, 1);
    }

    // update lru count of the inode
    ICACHE_LRU_INC(*inode);
    return 0;
}

uint64_t get_path_token_len(const char *path) {
    uint64_t len = 0;
    while (path[len] != '/' && path[len]) {
        len++;
    }
    return len;
}

uint32_t inode_get_idx_by_path(const char *path) {
    uint32_t inode_idx = 0;
    struct ext4_inode *inode;

    DEBUG("Looking up: %s", path);

    // first try to find in dcache
    struct decache_entry *dc_entry = decache_find(&path);
    inode_idx = dc_entry ? dc_entry->inode_idx : root->inode_idx;
    DEBUG("Found inode_idx %d, path = %s", inode_idx, path);

    do {
        uint64_t offset = 0;
        struct ext4_dir_entry_2 *de = NULL;

        path = skip_trailing_backslash(path);
        uint64_t path_len = get_path_token_len(path);

        if (path_len == 0) {
            // Reached the end of the path
            break;
        }

        // load inode by inode_idx
        inode_get_by_number(inode_idx, &inode);
        dcache_init(inode, inode_idx);
        while ((de = dentry_next(inode, inode_idx, offset))) {
            offset += de->rec_len;
            if (de->inode_idx == 0 && de->name_len == 0) {
                // reach the ext4_dir_entry_tail
                ASSERT(((struct ext4_dir_entry_tail *)de)->det_reserved_ft == EXT4_FT_DIR_CSUM);
                INFO("reach the last dentry");
                de = NULL;
                break;
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

            INFO("Add dir entry %s:%d to dentry cache", path, path_len);
            dc_entry = decache_insert(dc_entry, path, path_len, inode_idx);

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

int inode_get_by_path(const char *path, struct ext4_inode **inode, uint32_t *inode_idx) {
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
    ASSERT(inode_idx != 0);
    inode_idx--;
    // first calculate inode_idx in which group
    uint32_t group_idx = inode_idx / EXT4_INODES_PER_GROUP(sb);
    ASSERT(group_idx < EXT4_N_BLOCK_GROUPS(sb));

    // get inode table offset by gdt
    uint64_t off = EXT4_DESC_INODE_TABLE(gdt[group_idx]);
    off = BLOCKS2BYTES(off) + (inode_idx % EXT4_INODES_PER_GROUP(sb)) * EXT4_S_INODE_SIZE(sb);
    DEBUG("inode_idx: %u, group_idx: %u, off: %lu", inode_idx + 1, group_idx, off);
    return off;
}

int inode_check_permission(struct ext4_inode *inode, access_mode_t mode) {
    // UNIX permission check
    struct fuse_context *cntx = fuse_get_context();  //获取当前用户的上下文
    uid_t uid = cntx ? cntx->uid : 0;
    gid_t gid = cntx ? cntx->gid : 0;
    uid_t i_uid = EXT4_INODE_GET_UID(inode);
    gid_t i_gid = EXT4_INODE_GET_GID(inode);
    DEBUG("check inode & user permission on op mode %d", mode);
    DEBUG("inode uid %d, inode gid %d", i_uid, i_gid);
    DEBUG("user uid %d, user gid %d", uid, gid);

    // allow for root
    if (uid == 0) {
        INFO("Permission granted");
        return 0;
    }

    // user check
    if (i_uid == uid) {
        if ((mode == READ && (inode->i_mode & S_IRUSR)) || (mode == WRITE && (inode->i_mode & S_IWUSR)) ||
            (mode == RDWR && ((inode->i_mode & S_IRUSR) && (inode->i_mode & S_IWUSR))) ||
            (mode == EXEC && (inode->i_mode & S_IXUSR))) {
            INFO("Permission granted");
            return 0;
        }
    }

    // group check
    if (i_gid == gid) {
        if ((mode == READ && (inode->i_mode & S_IRGRP)) || (mode == WRITE && (inode->i_mode & S_IWGRP)) ||
            (mode == RDWR && ((inode->i_mode & S_IRGRP) && (inode->i_mode & S_IWGRP))) ||
            (mode == EXEC && (inode->i_mode & S_IXGRP))) {
            INFO("Permission granted");
            return 0;
        }
    } else {
        // other check
        if ((mode == READ && (inode->i_mode & S_IROTH)) || (mode == WRITE && (inode->i_mode & S_IWOTH)) ||
            (mode == RDWR && ((inode->i_mode & S_IROTH) && (inode->i_mode & S_IWOTH))) ||
            (mode == EXEC && (inode->i_mode & S_IXOTH))) {
            INFO("Permission granted");
            return 0;
        }
    }

    INFO("Permission denied");
    return -EACCES;
}

// find the path's directory inode_idx
uint32_t inode_get_parent_idx_by_path(const char *path) {
    char *tmp = strdup(path);
    char *last_slashp = strrchr(tmp, '/');
    if (last_slashp == path) {
        free(tmp);
        return 0;
    }
    *(last_slashp + 1) = '\0';
    uint32_t parent_idx = inode_get_idx_by_path(tmp);
    free(tmp);
    return parent_idx;
}

int inode_get_parent_by_path(const char *path, struct ext4_inode **inode, uint32_t *inode_idx) {
    uint32_t parent_idx = inode_get_parent_idx_by_path(path);
    if (parent_idx == 0) {
        return -ENOENT;
    }
    if (inode_idx != NULL) {
        *inode_idx = parent_idx;
    }
    return inode_get_by_number(parent_idx, inode);
}

int inode_create(uint32_t inode_idx, mode_t mode, struct ext4_inode **inode) {
    *inode = icache_insert(inode_idx, 0);
    memset(*inode, 0, sizeof(struct ext4_inode));
    (*inode)->i_mode = mode;
    time_t now = time(NULL);
    (*inode)->i_atime = (*inode)->i_ctime = (*inode)->i_mtime = now;
    struct fuse_context *cntx = fuse_get_context();
    uid_t uid = cntx ? cntx->uid : 0;
    gid_t gid = cntx ? cntx->gid : 0;
    EXT4_INODE_SET_UID(*inode, uid);
    EXT4_INODE_SET_GID(*inode, gid);

    // dir has 2 links and others have 1 link
    (*inode)->i_links_count = mode & S_IFDIR ? 2 : 1;

    // new created inode has 0 block and 0 size
    EXT4_INODE_SET_BLOCKS(*inode, 0);
    uint64_t inode_size = mode & S_IFDIR ? BLOCK_SIZE : 0;
    EXT4_INODE_SET_SIZE(*inode, inode_size);

    // enable ext4 extents flag
    (*inode)->i_flags |= EXT4_EXTENTS_FL;

    // ext4 extent header in block[0-3]
    struct ext4_extent_header *eh = (struct ext4_extent_header *)((*inode)->i_block);
    eh->eh_magic = EXT4_EXT_MAGIC;
    eh->eh_entries = 0;
    eh->eh_max = EXT4_EXT_LEAF_EH_MAX;
    eh->eh_depth = 0;
    eh->eh_generation = EXT4_EXT_EH_GENERATION;

    // new inode has one ext4 extent
    struct ext4_extent *ee = (struct ext4_extent *)(eh + 1);
    memset(ee, 0, 4 * sizeof(struct ext4_extent));
    ICACHE_SET_DIRTY(*inode);  // mark new inode as dirty
    INFO("new inode %u created", inode_idx);
    return 0;
}

int inode_mode2type(mode_t mode) {
    if (mode & S_IFDIR) {
        return EXT4_FT_DIR;
    } else if (mode & S_IFREG) {
        return EXT4_FT_REG_FILE;
    } else if (mode & S_IFLNK) {
        return EXT4_FT_SYMLINK;
    } else if (mode & S_IFCHR) {
        return EXT4_FT_CHRDEV;
    } else if (mode & S_IFBLK) {
        return EXT4_FT_BLKDEV;
    } else if (mode & S_IFIFO) {
        return EXT4_FT_FIFO;
    } else if (mode & S_IFSOCK) {
        return EXT4_FT_SOCK;
    } else {
        return EXT4_FT_UNKNOWN;
    }
}

int inode_init_pblock(struct ext4_inode *inode, uint64_t pblock_idx) {
    DEBUG("init inode with pblock [%d - %d]", pblock_idx, pblock_idx + EXT4_INODE_PBLOCK_NUM - 1);
    struct ext4_extent_header *eh = (struct ext4_extent_header *)inode->i_block;
    eh->eh_entries = 1;
    struct ext4_extent *ee = (struct ext4_extent *)(eh + 1);

    ee->ee_block = 0;  // logical block 0
    EXT4_EXT_SET_PADDR(ee, pblock_idx);
    ee->ee_len = EXT4_INODE_PBLOCK_NUM;

    EXT4_INODE_SET_BLOCKS(inode, EXT4_INODE_PBLOCK_NUM);
    ICACHE_SET_DIRTY(inode);
    bitmap_pblock_set(pblock_idx, EXT4_INODE_PBLOCK_NUM, 1);
    return 0;
}