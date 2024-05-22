
#pragma once

#include <stdint.h>

#include "ext4/ext4.h"

int bitmap_init();

/**
 * @brief find a free inode in inode bitmap
 *
 * @param inode_idx parent inode_idx
 * @return uint32_t inode_idx, 0 if no free inode
 */
uint32_t bitmap_inode_find(uint32_t inode_idx);

/**
 * @brief find a free block in block bitmap
 * 
 * @param inode_idx 
 * @return uint64_t 0 if no free block
 */
uint64_t bitmap_block_find(uint32_t inode_idx);