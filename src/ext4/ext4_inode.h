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

#ifndef EXT4_INODE_H
#define EXT4_INODE_H

#include "ext4_basic.h"

#define EXT4_NDIR_BLOCKS 12
#define EXT4_IND_BLOCK EXT4_NDIR_BLOCKS
#define EXT4_DIND_BLOCK (EXT4_IND_BLOCK + 1)
#define EXT4_TIND_BLOCK (EXT4_DIND_BLOCK + 1)
#define EXT4_N_BLOCKS (EXT4_TIND_BLOCK + 1)

#define EXT4_SECRM_FL 0x00000001     /* Secure deletion */
#define EXT4_UNRM_FL 0x00000002      /* Undelete */
#define EXT4_COMPR_FL 0x00000004     /* Compress file */
#define EXT4_SYNC_FL 0x00000008      /* Synchronous updates */
#define EXT4_IMMUTABLE_FL 0x00000010 /* Immutable file */
#define EXT4_APPEND_FL 0x00000020    /* writes to file may only append */
#define EXT4_NODUMP_FL 0x00000040    /* do not dump file */
#define EXT4_NOATIME_FL 0x00000080   /* do not update atime */
/* Reserved for compression usage... */
#define EXT4_DIRTY_FL 0x00000100
#define EXT4_COMPRBLK_FL 0x00000200 /* One or more compressed clusters */
#define EXT4_NOCOMPR_FL 0x00000400  /* Don't compress */
#define EXT4_ECOMPR_FL 0x00000800   /* Compression error */
/* End compression flags --- maybe not all used */
#define EXT4_INDEX_FL 0x00001000        /* hash-indexed directory */
#define EXT4_IMAGIC_FL 0x00002000       /* AFS directory */
#define EXT4_JOURNAL_DATA_FL 0x00004000 /* file data should be journaled */
#define EXT4_NOTAIL_FL 0x00008000       /* file tail should not be merged */
#define EXT4_DIRSYNC_FL 0x00010000      /* dirsync behaviour (directories only) */
#define EXT4_TOPDIR_FL 0x00020000       /* Top of directory hierarchies*/
#define EXT4_HUGE_FILE_FL 0x00040000    /* Set to each huge file */
#define EXT4_EXTENTS_FL 0x00080000      /* Inode uses extents */
#define EXT4_EA_INODE_FL 0x00200000     /* Inode used for large EA */
#define EXT4_EOFBLOCKS_FL 0x00400000    /* Blocks allocated beyond EOF */
#define EXT4_RESERVED_FL 0x80000000     /* reserved for ext4 lib */

// https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout#Inode_Table
struct ext4_inode {
    __le16 i_mode;        /* 文件模式,包括文件类型和文件的访问权限 */
    __le16 i_uid;         /* 文件所有者的低16位用户ID */
    __le32 i_size_lo;     /* 文件大小的低32位 */
    __le32 i_atime;       /* 文件最后访问时间 */
    __le32 i_ctime;       /* inode最后改变时间 */
    __le32 i_mtime;       /* 文件最后修改时间 */
    __le32 i_dtime;       /* 文件删除时间 */
    __le16 i_gid;         /* 文件所有组的低16位组ID */
    __le16 i_links_count; /* 指向该inode的硬链接数量 */
    __le32 i_blocks_lo;   /* 文件占用的块数的低32位 */
    __le32 i_flags;       /* 文件标志,用于存储文件的特定属性 */
    union {
        struct {
            __le32 l_i_version; /* 用于NFS的文件版本号 */
        } linux1;
        struct {
            __u32 h_i_translator; /* Hurd的特定字段 */
        } hurd1;
        struct {
            __u32 m_i_reserved1; /* Masix的保留字段 */
        } masix1;
    } osd1;                        /* 操作系统依赖字段1 */
    __le32 i_block[EXT4_N_BLOCKS]; /* 指向文件数据块的指针数组 */
    __le32 i_generation;           /* 文件版本号,用于NFS */
    __le32 i_file_acl_lo;          /* 文件访问控制列表的低32位 */
    __le32 i_size_high;            /* 文件大小的高32位 */
    __le32 i_obso_faddr;           /* 已废弃的碎片地址 */
    union {
        struct {
            __le16 l_i_blocks_high;   /* 文件占用块数的高16位 */
            __le16 l_i_file_acl_high; /* 文件ACL的高16位 */
            __le16 l_i_uid_high;      /* 用户ID的高16位 */
            __le16 l_i_gid_high;      /* 组ID的高16位 */
            __u32 l_i_reserved2;      /* 保留字段 */
        } linux2;
        struct {
            __le16 h_i_reserved1; /* 已废弃的碎片编号/大小 */
            __u16 h_i_mode_high;  /* 文件模式的高16位 */
            __u16 h_i_uid_high;   /* 用户ID的高16位 */
            __u16 h_i_gid_high;   /* 组ID的高16位 */
            __u32 h_i_author;     /* Hurd的作者字段 */
        } hurd2;
        struct {
            __le16 m_i_reserved1;     /* 已废弃的碎片编号/大小 */
            __le16 m_i_file_acl_high; /* 文件ACL的高16位 */
            __u32 m_i_reserved2[2];   /* Masix的保留字段 */
        } masix2;
    } osd2;                /* 操作系统依赖字段2 */
    __le16 i_extra_isize;  /* inode结构体扩展大小 */
    __le16 i_pad1;         /* 对齐填充 */
    __le32 i_ctime_extra;  /* inode最后改变时间的额外信息(纳秒和纪元) */
    __le32 i_mtime_extra;  /* 文件最后修改时间的额外信息(纳秒和纪元) */
    __le32 i_atime_extra;  /* 文件最后访问时间的额外信息(纳秒和纪元) */
    __le32 i_crtime;       /* 文件创建时间 */
    __le32 i_crtime_extra; /* 文件创建时间的额外信息(纳秒和纪元) */
    __le32 i_version_hi;   /* 64位版本号的高32位 */
};

#endif
