
#include "bitmap.h"

#include <stdint.h>

#include "ext4/ext4.h"
#include "logging.h"

extern struct ext4_super_block sb;
extern struct ext4_group_desc *gdt;

/**
 * @brief find the first free inode in inode bitmap
 *
 * @param inode_idx parent inode_idx
 * @return uint32_t inode_idx, 0 if no free inode
 */
uint32_t bitmap_inode_find(uint32_t inode_idx) {
    uint32_t group_idx = inode_idx / EXT4_INODES_PER_GROUP(sb);
    uint32_t group_num = EXT4_N_BLOCK_GROUPS(sb);
    ASSERT(group_idx < group_num);

    DEBUG("finding free inode in group %u", group_idx);
    char *inode_bitmap = (char *)BLOCKS2BYTES(EXT4_DESC_INO_BITMAP(gdt[group_idx]));

    uint32_t new_inode_idx = 0;
    for (uint32_t i = 0; i < EXT4_INODES_PER_GROUP(sb); i++) {
        if (BIT(inode_bitmap, i) == 0) {
            new_inode_idx = (group_idx * EXT4_INODES_PER_GROUP(sb)) + i;
            INFO("found free inode %u", new_inode_idx);
            return new_inode_idx;
        }
    }
    INFO("no free inode in group %u", group_idx);
    // loop through all groups to search a free inode
    for (uint32_t i = group_idx + 1; i != group_idx; i = (i + 1) % group_num) {
        DEBUG("finding free inode in group %u", i);
        inode_bitmap = (char *)BLOCKS2BYTES(EXT4_DESC_INO_BITMAP(gdt[i]));
        for (uint32_t j = 0; j < EXT4_INODES_PER_GROUP(sb); j++) {
            if (BIT(inode_bitmap, j) == 0) {
                new_inode_idx = (i * EXT4_INODES_PER_GROUP(sb)) + j;
                INFO("found free inode %u", new_inode_idx);
                return new_inode_idx;
            }
        }
    }

    ERR("no free inode");
    return new_inode_idx;
}