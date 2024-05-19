/* vim: set ts=8 :
 *
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 *
 *  from
 *
 *  linux/fs/ext4/ext4_extents.h
 *
 * Copyright (c) 2003-2006, Cluster File Systems, Inc, info@clusterfs.com
 * Written by Alex Tomas <alex@clusterfs.com>
 */

#ifndef EXT4_EXTENTS_H
#define EXT4_EXTENTS_H

#include "ext4_basic.h"

/*
 * ext4_inode has i_block array (60 bytes total).
 * The first 12 bytes store ext4_extent_header;
 * the remainder stores an array of ext4_extent.
 */

/*
 * This is the extent on-disk structure.
 * It's used at the bottom of the tree.
 */
struct ext4_extent {
    __le32 ee_block;    /* 文件逻辑块号的第一个块,表示这个扩展的起始块 */
    __le16 ee_len;      /* 这个扩展覆盖的数据块数量 */
    __le16 ee_start_hi; /* 物理块号的高 16 位 */
    __le32 ee_start_lo; /* 物理块号的低 32 位 */
};
/*
 * This is index on-disk structure.
 * It's used at all the levels except the bottom.
 */
struct ext4_extent_idx {
    __le32 ei_block;   /* index covers logical blocks from 'block' */
    __le32 ei_leaf_lo; /* pointer to the physical block of the next *
                        * level. leaf or next index could be there */
    __le16 ei_leaf_hi; /* high 16 bits of physical block */
    __u16 ei_unused;
};

/*
 * Each block (leaves and indexes), even inode-stored has header.
 */
struct ext4_extent_header {
    __le16 eh_magic;      /* 魔数,用于识别扩展索引的格式 */
    __le16 eh_entries;    /* 有效的条目数量,表示当前有多少条目在使用 */
    __le16 eh_max;        /* 存储空间的最大容量,以条目数计 */
    __le16 eh_depth;      /* 树的深度,表示是否有实际的底层块 */
    __le32 eh_generation; /* 扩展索引树的版本号,用于确保一致性 */
};

#define EXT4_EXT_MAGIC 0xF30A // extent header magic number

#endif
