/*
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

#include "super.h"

#include <stdint.h>

#include "disk.h"
#include "ext4/ext4_super.h"
#include "logging.h"

struct ext4_super_block sb;
struct ext4_group_desc *gdesc_table;

int super_fill(void) {
    disk_read(BOOT_SECTOR_SIZE, sizeof(struct ext4_super_block), &sb);

    INFO("BLOCK SIZE: %i", EXT4_BLOCK_SIZE(sb));
    INFO("BLOCK GROUP SIZE: %i", EXT4_SUPER_GROUP_SIZE(sb));
    INFO("N BLOCK GROUPS: %i", EXT4_N_BLOCK_GROUPS(sb));
    INFO("INODE SIZE: %i", EXT4_S_INODE_SIZE(sb));
    INFO("INODES PER GROUP: %i", EXT4_INODES_PER_GROUP(sb));
    INFO("GROUP DESC SIZE: %i", EXT4_DESC_SIZE(sb));
    return 0;
}

int super_group_fill(void) {
    int group_num = EXT4_N_BLOCK_GROUPS(sb);
    gdesc_table = malloc(sizeof(struct ext4_group_desc) * group_num);

    for (uint32_t i = 0; i < group_num; i++) {
        off_t bg_off = ALIGN_TO_BLOCKSIZE(BOOT_SECTOR_SIZE + sizeof(struct ext4_super_block));
        bg_off += i * EXT4_DESC_SIZE(sb);

        /* disk advances EXT4_DESC_SIZE(sb), pointer sizeof(struct...).
         * These values might be different!!! */
        disk_read(bg_off, EXT4_DESC_SIZE(sb), &gdesc_table[i]);
    }

    return 0;
}
