/*
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

#include <stdint.h>
#include <stdlib.h>

#include "disk.h"
#include "ext4/ext4.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"
unsigned fuse_capable;

struct ext4_super_block sb;
struct ext4_group_desc *gdt;

int super_fill(void) {
    disk_read(BOOT_SECTOR_SIZE, sizeof(struct ext4_super_block), &sb);

    INFO("BLOCK SIZE: %i", EXT4_BLOCK_SIZE(sb));
    INFO("BLOCK GROUP SIZE: %i", EXT4_SUPER_GROUP_SIZE(sb));
    INFO("N BLOCK GROUPS: %i", EXT4_N_BLOCK_GROUPS(sb));
    INFO("INODE SIZE: %i", EXT4_S_INODE_SIZE(sb));
    INFO("INODES PER GROUP: %i", EXT4_INODES_PER_GROUP(sb));
    INFO("GROUP DESC SIZE: %i", EXT4_DESC_SIZE(sb));

    time_t now = time(NULL);
    sb.s_mtime = now;
    // DEBUG("mount time is %s", ctime(&now));
    sb.s_mnt_count += 1;
    return 0;
}

int super_group_fill(void) {
    int group_num = EXT4_N_BLOCK_GROUPS(sb);
    gdt = malloc(sizeof(struct ext4_group_desc) * group_num);
    off_t bg_off = ALIGN_TO_BLOCKSIZE(BOOT_SECTOR_SIZE + sizeof(struct ext4_super_block));

    for (uint32_t i = 0; i < group_num; i++) {
        /* disk advances EXT4_DESC_SIZE(sb), pointer sizeof(struct...).
         * These values might be different!!! */
        disk_read(bg_off, EXT4_DESC_SIZE(sb), &gdt[i]);
        bg_off += EXT4_DESC_SIZE(sb);
    }
    return 0;
}

void *op_init(struct fuse_conn_info *info, struct fuse_config *cfg) {
    INFO("Using FUSE protocol %d.%d", info->proto_major, info->proto_minor);
    cfg->kernel_cache = 1;  // Enable kernel cache
    fuse_capable = info->capable;
    // Initialize the super block
    super_fill();        // superblock
    super_group_fill();  // group descriptors
    inode_init();        // root inode

    return NULL;
}
