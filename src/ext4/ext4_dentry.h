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

#define EXT4_NAME_LEN 255

struct ext4_dir_entry_2 {
    __le32 inode;             /* Inode number: 存储文件的索引节点号 */
    __le16 rec_len;           /* Directory entry length: 存储目录项的总长度 */
    __u8 name_len;            /* Name length: 存储文件名的长度 */
    __u8 file_type;           /* File type: 存储文件类型 */
    char name[EXT4_NAME_LEN]; /* File name: 存储文件名,最大长度由EXT4_NAME_LEN定义 */
};

#endif
