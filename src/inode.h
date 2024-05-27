#ifndef INODE_H
#define INODE_H

#include <stdint.h>
#include <sys/types.h>

#include "bitmap.h"
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

#define IS_PATH_SEPARATOR(__c)  ((__c) == '/')

uint8_t get_path_token_len(const char *path);
const char *skip_trailing_backslash(const char *path);

uint64_t inode_get_data_pblock(struct ext4_inode *inode, uint32_t lblock, uint32_t *extent_len);
int inode_get_all_pblocks(struct ext4_inode *inode, struct pblock_arr *pblock_arr);

int inode_get_by_number(uint32_t n, struct ext4_inode **inode);
int inode_get_by_path(const char *path, struct ext4_inode **inode, uint32_t *inode_idx);
uint64_t inode_get_offset(uint32_t inode_idx);
uint32_t inode_get_idx_by_path(const char *path);

typedef enum { READ, WRITE, RDWR, EXEC } access_mode_t;

int inode_check_permission(struct ext4_inode *inode, access_mode_t mode);

// find a free inode
int inode_find_free();

/**
 * @brief create a new inode
 *
 * @param inode_idx
 * @param mode
 * @param pblock
 * @param inode
 * @return int
 */
int inode_create(uint32_t inode_idx, mode_t mode, uint64_t pblock, struct ext4_inode **inode);

uint32_t inode_get_parent_idx_by_path(const char *path);
int inode_get_parent_by_path(const char *path, struct ext4_inode **inode, uint32_t *inode_idx);
/**
 * @brief write back the inode to disk
 *
 * @param inode_idx
 * @param inode
 * @return int
 */
int inode_write_back(uint32_t inode_idx, struct ext4_inode *inode);

int inode_bitmap_has_space(uint32_t parent_idx, uint32_t *inode_idx, uint64_t *pblock);

int inode_mode2type(mode_t mode);

#endif
