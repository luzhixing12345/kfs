
#include "bitmap.h"

#include <stdint.h>
#include <stdlib.h>

#include "disk.h"
#include "ext4/ext4.h"
#include "logging.h"

int bitmap_init() {
    INFO("init bitmap");

    uint32_t group_num = EXT4_N_BLOCK_GROUPS(sb);
    i_bitmap = malloc(sizeof(uint64_t) * group_num);
    d_bitmap = malloc(sizeof(uint64_t) * group_num);
    uint64_t off;
    for (uint32_t i = 0; i < group_num; i++) {
        INFO("init inode & data bitmap for group %u", i);
        i_bitmap[i] = malloc(EXT4_INODES_PER_GROUP(sb) / 8);
        d_bitmap[i] = malloc(EXT4_BLOCKS_PER_GROUP(sb) / 8);

        off = BLOCKS2BYTES(EXT4_DESC_INO_BITMAP(gdt[i]));
        disk_read(off, EXT4_INODES_PER_GROUP(sb) / 8, i_bitmap[i]);
        off = BLOCKS2BYTES(EXT4_DESC_BLOCK_BITMAP(gdt[i]));
        disk_read(off, EXT4_BLOCKS_PER_GROUP(sb) / 8, d_bitmap[i]);
    }
    return 0;
}

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

    uint32_t new_inode_idx = 0;
    for (uint32_t i = 0; i < EXT4_INODES_PER_GROUP(sb); i++) {
        if (BIT(i_bitmap[group_idx], i) == 0) {
            new_inode_idx = (group_idx * EXT4_INODES_PER_GROUP(sb)) + i + 1;
            INFO("found free inode %u", new_inode_idx);
            return new_inode_idx;
        }
    }
    INFO("no free inode in group %u", group_idx);
    // loop through all groups to search a free inode
    for (uint32_t i = group_idx + 1; i != group_idx; i = (i + 1) % group_num) {
        DEBUG("finding free inode in group %u", i);
        for (uint32_t j = 0; j < EXT4_INODES_PER_GROUP(sb); j++) {
            if (BIT(i_bitmap[i], j) == 0) {
                new_inode_idx = (i * EXT4_INODES_PER_GROUP(sb)) + j + 1;
                INFO("found free inode %u", new_inode_idx);
                return new_inode_idx;
            }
        }
    }

    ERR("no free inode");
    return new_inode_idx;
}