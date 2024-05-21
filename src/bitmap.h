
#pragma once

#include <stdint.h>

#include "ext4/ext4_super.h"

/**
 * @brief find the first free inode in inode bitmap
 *
 * @param inode_idx parent inode_idx
 * @return uint32_t inode_idx, 0 if no free inode
 */
uint32_t bitmap_inode_find(uint32_t inode_idx);