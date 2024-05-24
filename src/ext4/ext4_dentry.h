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
 *  linux/fs/ext4/ext4.h
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/include/linux/minix_fs.h
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 */

#ifndef EXT4_DENTRY_H
#define EXT4_DENTRY_H

#include "ext4_basic.h"

#define EXT4_NAME_LEN     255
#define EXT4_DE_BASE_SIZE 8

// https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout#Directory_Entries
struct ext4_dir_entry_2 {
    __le32 inode_idx;         /* Inode number: 存储文件的索引节点号 */
    __le16 rec_len;           /* Directory entry length: 存储目录项的总长度 */
    __u8 name_len;            /* Name length: 存储文件名的长度 */
    __u8 file_type;           /* File type: 存储文件类型 */
    char name[EXT4_NAME_LEN]; /* File name: 存储文件名,最大长度由EXT4_NAME_LEN定义 */
};

// 	File type code, one of:
// 0x0	Unknown.
// 0x1	Regular file.
// 0x2	Directory.
// 0x3	Character device file.
// 0x4	Block device file.
// 0x5	FIFO.
// 0x6	Socket.
// 0x7	Symbolic link.

#define EXT4_FT_UNKNOWN  0
#define EXT4_FT_REG_FILE 1
#define EXT4_FT_DIR      2
#define EXT4_FT_CHRDEV   3
#define EXT4_FT_BLKDEV   4
#define EXT4_FT_FIFO     5
#define EXT4_FT_SOCK     6
#define EXT4_FT_SYMLINK  7

/*
 * EXT4_DIR_PAD defines the directory entries boundaries
 *
 * NOTE: It must be a multiple of 4
 */
#define EXT4_DIR_PAD     4
#define EXT4_DIR_ROUND   (EXT4_DIR_PAD - 1)
#define EXT4_MAX_REC_LEN ((1 << 16) - 1)

// an entry tail will be placed at the end of a directory entry
// it is used to store the checksum of the entry
// it's always 12 bytes long, and

// https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout#Directory_Entries
struct ext4_dir_entry_tail {
    __le32 det_reserved_zero1;  // Inode number, which must be zero.
    __le16 det_rec_len;         // Length of this directory entry, which must be 12
    __u8 det_reserved_zero2;    // Length of the file name, which must be zero.
    __u8 det_reserved_ft;       // File type, which must be 0xDE.
    __le32 det_checksum;        // Directory leaf block checksum.
};

#define EXT4_FT_DIR_CSUM  0xDE
#define EXT4_DE_TAIL_SIZE 12
#define EXT4_DE_DOT_SIZE  12  // . ..

#endif
