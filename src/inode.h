#ifndef INODE_H
#define INODE_H

#include <stdint.h>
#include <sys/types.h>

#include "common.h"
#include "ext4/ext4_dentry.h"
#include "ext4/ext4_inode.h"

/* These #defines are only relevant for ext2/3 style block indexing */
#define ADDRESSES_IN_IND_BLOCK  (BLOCK_SIZE / sizeof(uint32_t))
#define ADDRESSES_IN_DIND_BLOCK (ADDRESSES_IN_IND_BLOCK * ADDRESSES_IN_IND_BLOCK)
#define ADDRESSES_IN_TIND_BLOCK (ADDRESSES_IN_DIND_BLOCK * ADDRESSES_IN_IND_BLOCK)
#define MAX_IND_BLOCK           (EXT4_NDIR_BLOCKS + ADDRESSES_IN_IND_BLOCK)
#define MAX_DIND_BLOCK          (MAX_IND_BLOCK + ADDRESSES_IN_DIND_BLOCK)
#define MAX_TIND_BLOCK          (MAX_DIND_BLOCK + ADDRESSES_IN_TIND_BLOCK)

#define ROOT_INODE_N            2  // root inode_idx
#define IS_PATH_SEPARATOR(__c)  ((__c) == '/')

// inode directory context
struct inode_dir_ctx {
    uint32_t inode_idx;
    uint32_t lblock; /* Currently buffered lblock */
    uint8_t buf[];
};

uint64_t inode_get_data_pblock(struct ext4_inode *inode, uint32_t lblock, uint32_t *extent_len);

struct inode_dir_ctx *dir_ctx_malloc(void);
void dir_ctx_free(struct inode_dir_ctx *);
void dir_ctx_init(struct inode_dir_ctx *ctx, struct ext4_inode *inode);

int inode_get_by_number(uint32_t n, struct ext4_inode *inode);
int inode_get_by_path(const char *path, struct ext4_inode *inode);
uint64_t inode_get_offset(uint32_t inode_idx);
uint32_t inode_get_idx_by_path(const char *path);

struct dcache_entry *get_cached_inode_idx(const char **path);

typedef enum { RD_ONLY, WR_ONLY, RDWR } access_mode_t;

int inode_check_permission(struct ext4_inode *inode, access_mode_t mode);

// find a free inode
int inode_find_free();

uint32_t inode_get_parent_idx_by_path(const char *path);

#endif
