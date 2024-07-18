
#pragma once

#include <stdint.h>

#include "ext4/ext4.h"

struct bitmap {
    struct bitmap_group *group;
    uint32_t group_num;
};

struct bitmap_group {
    uint8_t *bitmap;  // bitmap of each block group
    uint64_t off;     // offset of each block group
    int status;       // 0: valid, 1: dirty
};

struct pblock_range {
    uint64_t pblock;  // start of pblock
    uint16_t len;     // length of pblock
};

struct pblock_arr {
    struct pblock_range *arr;
    uint16_t len;
};

#define BITMAP_S_VALID 0
#define BITMAP_S_DIRTY 1

int bitmap_init();

/**
 * @brief find a free inode in inode bitmap
 *
 * @param inode_idx parent inode_idx
 * @return uint32_t inode_idx, 0 if no free inode
 */
uint32_t bitmap_inode_find(uint32_t inode_idx);

/**
 * @brief set inode bitmap to 1/0
 *
 * @param inode_idx
 * @param is_used
 * @return int
 */
int bitmap_inode_set(uint32_t inode_idx, int is_used);

/**
 * @brief find a free block in block bitmap
 *
 * @param inode_idx
 * @return uint64_t 0 if no free block
 */
uint64_t bitmap_pblock_find(uint32_t inode_idx);

/**
 * @brief find a free inode and data block for new inode
 * 
 * @param parent_idx 
 * @param inode_idx 
 * @param pblock 
 * @return int 0 if success, <0 if fail
 */
int bitmap_find_space(uint32_t parent_idx, uint32_t *inode_idx, uint64_t *pblock);

/**
 * @brief set block bitmap to 1/0
 *
 * @param block_idx
 * @param is_used
 * @return int
 */
int bitmap_pblock_set(uint64_t block_idx, int is_used);
int bitmap_pblock_free(struct pblock_arr *arr);
void gdt_update(uint32_t inode_idx);