
#include "bitmap.h"

#include <stdint.h>
#include <stdlib.h>

#include "disk.h"
#include "ext4/ext4.h"
#include "logging.h"

struct bitmap i_bitmap;  // inode bitmap
struct bitmap d_bitmap;  // data bitmap

int bitmap_init() {
    INFO("init bitmap");

    uint32_t group_num = EXT4_N_BLOCK_GROUPS(sb);
    i_bitmap.group_num = group_num;
    d_bitmap.group_num = group_num;
    i_bitmap.group = calloc(group_num, sizeof(struct bitmap_group));
    d_bitmap.group = calloc(group_num, sizeof(struct bitmap_group));
    uint64_t off;
    uint64_t size;
    for (uint32_t i = 0; i < group_num; i++) {
        INFO("init inode & data bitmap for group %u", i);
        off = BLOCKS2BYTES(EXT4_DESC_INO_BITMAP(gdt[i]));
        size = EXT4_INODES_PER_GROUP(sb) / 8;
        i_bitmap.group[i].bitmap = malloc(size);
        disk_read(off, size, i_bitmap.group[i].bitmap);
        i_bitmap.group[i].off = off;

        off = BLOCKS2BYTES(EXT4_DESC_BLOCK_BITMAP(gdt[i]));
        size = EXT4_BLOCKS_PER_GROUP(sb) / 8;
        d_bitmap.group[i].bitmap = malloc(size);
        disk_read(off, size, d_bitmap.group[i].bitmap);
        d_bitmap.group[i].off = off;
    }
    return 0;
}

/**
 * @brief find a free inode in inode bitmap
 *
 * @param inode_idx parent inode_idx
 * @return uint32_t inode_idx, 0 if no free inode
 */
uint32_t bitmap_inode_find(uint32_t inode_idx) {
    uint32_t group_idx = inode_idx / EXT4_INODES_PER_GROUP(sb);
    uint32_t group_num = EXT4_N_BLOCK_GROUPS(sb);
    ASSERT(group_idx < group_num);

    DEBUG("finding free inode in group %u", group_idx);

    // TODO: better inode select
    // for now, just simply find the first free inode in i_bitmap
    uint32_t new_inode_idx = 0;
    for (uint32_t i = 0; i < EXT4_INODES_PER_GROUP(sb); i++) {
        if (BIT0(i_bitmap.group[group_idx].bitmap, i)) {
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
            if (BIT0(i_bitmap.group[i].bitmap, j)) {
                new_inode_idx = (i * EXT4_INODES_PER_GROUP(sb)) + j + 1;
                INFO("found free inode %u", new_inode_idx);
                return new_inode_idx;
            }
        }
    }

    ERR("no free inode");
    return new_inode_idx;
}

int bitmap_inode_set(uint32_t inode_idx, int is_used) {
    ASSERT(inode_idx != 0);
    inode_idx--;
    uint32_t group_idx = inode_idx / EXT4_INODES_PER_GROUP(sb);
    uint32_t index = inode_idx % EXT4_INODES_PER_GROUP(sb);
    uint32_t group_num = EXT4_N_BLOCK_GROUPS(sb);
    ASSERT(group_idx < group_num);
    SET_BIT(i_bitmap.group[group_idx].bitmap, index, is_used);
    i_bitmap.group[group_idx].status = BITMAP_S_DIRTY;
    return 0;
}

uint64_t bitmap_pblock_find(uint32_t inode_idx) {
    uint32_t group_idx = inode_idx / EXT4_INODES_PER_GROUP(sb);
    uint32_t group_num = EXT4_N_BLOCK_GROUPS(sb);
    ASSERT(group_idx < group_num);

    uint64_t new_block_idx = 0;
    for (uint32_t i = 0; i < EXT4_BLOCKS_PER_GROUP(sb); i++) {
        if (BIT0(d_bitmap.group[group_idx].bitmap, i)) {
            new_block_idx = (group_idx * EXT4_BLOCKS_PER_GROUP(sb)) + i;
            INFO("found free block %u", new_block_idx);
            return new_block_idx;
        }
    }

    INFO("no free block in group %u", group_idx);
    for (uint32_t i = group_idx + 1; i != group_idx; i = (i + 1) % group_num) {
        DEBUG("finding free block in group %u", i);
        for (uint32_t j = 0; j < EXT4_BLOCKS_PER_GROUP(sb); j++) {
            if (BIT0(d_bitmap.group[i].bitmap, j)) {
                new_block_idx = (i * EXT4_BLOCKS_PER_GROUP(sb)) + j;
                INFO("found free block %u", new_block_idx);
                return new_block_idx;
            }
        }
    }

    ERR("no free block");
    return UINT64_MAX;
}

int bitmap_pblock_set(uint64_t block_idx, int is_used) {
    uint32_t group_idx = block_idx / EXT4_BLOCKS_PER_GROUP(sb);
    uint32_t index = block_idx % EXT4_BLOCKS_PER_GROUP(sb);
    uint32_t group_num = EXT4_N_BLOCK_GROUPS(sb);
    ASSERT(group_idx < group_num);
    SET_BIT(d_bitmap.group[group_idx].bitmap, index, is_used);
    d_bitmap.group[group_idx].status = BITMAP_S_DIRTY;
    return 0;
}

// TODO: better set 0 in range instead of for loop
int bitmap_pblock_free(struct pblock_arr *p_arr) {
    struct pblock_range *range;
    for (uint16_t i = 0; i < p_arr->len; i++) {
        range = &p_arr->arr[i];
        for (uint16_t j = 0; j < range->len; j++) {
            INFO("free pblock %u", range->pblock + j);
            bitmap_pblock_set(range->pblock + j, 0);
        }
    }
    if (p_arr->len) {
        free(p_arr->arr);
        INFO("free pblocks done");
    }
    return 0;
}

int bitmap_find_space(uint32_t parent_idx, uint32_t *inode_idx, uint64_t *pblock) {
    // find a valid inode_idx in GDT
    *inode_idx = bitmap_inode_find(parent_idx);
    if (*inode_idx == 0) {
        ERR("No free inode");
        return -ENOSPC;
    }

    if (!pblock) {
        // short symlink inode does not need pblock, just return inode_idx
        INFO("no pblock needed");
        INFO("new dentry [inode:%u]", *inode_idx);
    } else {
        // find an empty data block
        *pblock = bitmap_pblock_find(*inode_idx);
        if (*pblock == UINT64_MAX) {
            ERR("No free block");
            return -ENOSPC;
        }
        INFO("new dentry [inode:%u] [pblock:%u]", *inode_idx, *pblock);
    }

    return 0;
}

void gdt_update(uint32_t inode_idx) {
    int group_idx = inode_idx / EXT4_INODES_PER_GROUP(sb);
    uint32_t free_blocks = EXT4_GDT_FREE_BLOCKS(&gdt[group_idx]) - 1;
    gdt[group_idx].bg_free_blocks_count_lo = free_blocks & MASK_16;
    gdt[group_idx].bg_free_blocks_count_hi = free_blocks >> 16;

    uint32_t free_inodes = EXT4_GDT_FREE_INODES(&gdt[group_idx]) - 1;
    gdt[group_idx].bg_free_inodes_count_lo = free_inodes & MASK_16;
    gdt[group_idx].bg_free_inodes_count_hi = free_inodes >> 16;

    // used_dirs may overflow
    uint32_t used_dirs = EXT4_GDT_USED_DIRS(&gdt[group_idx]);
    if (used_dirs == EXT4_GDT_MAX_USED_DIRS) {
        ERR("used dirs overflow!");
    } else {
        used_dirs++;
    }
    gdt[group_idx].bg_used_dirs_count_lo = used_dirs & MASK_16;
    gdt[group_idx].bg_used_dirs_count_hi = used_dirs >> 16;

    uint32_t unused_inodes = EXT4_GDT_ITABLE_UNUSED(&gdt[group_idx]) - 1;
    gdt[group_idx].bg_itable_unused_lo = unused_inodes & MASK_16;
    gdt[group_idx].bg_itable_unused_hi = unused_inodes >> 16;
    EXT4_GDT_SET_DIRTY(&gdt[group_idx]);  // set gdt dirty

    INFO("update gdt, free blocks %u, free inodes %u, used dirs %u, unused inodes %u",
         free_blocks,
         free_inodes,
         used_dirs,
         unused_inodes);
}
