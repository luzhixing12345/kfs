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

#define EXT4_NDIR_BLOCKS     12
#define EXT4_IND_BLOCK       EXT4_NDIR_BLOCKS
#define EXT4_DIND_BLOCK      (EXT4_IND_BLOCK + 1)
#define EXT4_TIND_BLOCK      (EXT4_DIND_BLOCK + 1)
#define EXT4_N_BLOCKS        (EXT4_TIND_BLOCK + 1)

#define EXT4_SECRM_FL        0x00000001 /* Secure deletion */
#define EXT4_UNRM_FL         0x00000002 /* Undelete */
#define EXT4_COMPR_FL        0x00000004 /* Compress file */
#define EXT4_SYNC_FL         0x00000008 /* Synchronous updates */
#define EXT4_IMMUTABLE_FL    0x00000010 /* Immutable file */
#define EXT4_APPEND_FL       0x00000020 /* writes to file may only append */
#define EXT4_NODUMP_FL       0x00000040 /* do not dump file */
#define EXT4_NOATIME_FL      0x00000080 /* do not update atime */
/* Reserved for compression usage... */
#define EXT4_DIRTY_FL        0x00000100
#define EXT4_COMPRBLK_FL     0x00000200 /* One or more compressed clusters */
#define EXT4_NOCOMPR_FL      0x00000400 /* Don't compress */
#define EXT4_ECOMPR_FL       0x00000800 /* Compression error */
/* End compression flags --- maybe not all used */
#define EXT4_INDEX_FL        0x00001000 /* hash-indexed directory */
#define EXT4_IMAGIC_FL       0x00002000 /* AFS directory */
#define EXT4_JOURNAL_DATA_FL 0x00004000 /* file data should be journaled */
#define EXT4_NOTAIL_FL       0x00008000 /* file tail should not be merged */
#define EXT4_DIRSYNC_FL      0x00010000 /* dirsync behaviour (directories only) */
#define EXT4_TOPDIR_FL       0x00020000 /* Top of directory hierarchies*/
#define EXT4_HUGE_FILE_FL    0x00040000 /* Set to each huge file */
#define EXT4_EXTENTS_FL      0x00080000 /* Inode uses extents */
#define EXT4_EA_INODE_FL     0x00200000 /* Inode used for large EA */
#define EXT4_EOFBLOCKS_FL    0x00400000 /* Blocks allocated beyond EOF */
#define EXT4_RESERVED_FL     0x80000000 /* reserved for ext4 lib */

// https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout#Inode_Table
struct ext4_inode {
    __le16 i_mode;        /* File mode */
    __le16 i_uid;         /* Low 16 bits of Owner Uid */
    __le32 i_size_lo;     /* Lower 32-bits of size in bytes. */
    __le32 i_atime;       /* Access time */
    __le32 i_ctime;       /* Inode Change time */
    __le32 i_mtime;       /* Modification time */
    __le32 i_dtime;       /* Deletion Time */
    __le16 i_gid;         /* Low 16 bits of Group Id */
    __le16 i_links_count; /* Links count */
    __le32 i_blocks_lo;   /* Blocks count */
    __le32 i_flags;       /* File flags */
    union {
        struct {
            __le32 l_i_version;
        } linux1;
        struct {
            __u32 h_i_translator;
        } hurd1;
        struct {
            __u32 m_i_reserved1;
        } masix1;
    } osd1;                        /* OS dependent 1 */
    __le32 i_block[EXT4_N_BLOCKS]; /* Pointers to blocks */
    __le32 i_generation;           /* File version (for NFS) */
    __le32 i_file_acl_lo;          /* File ACL */
    __le32 i_size_high;
    __le32 i_obso_faddr; /* Obsoleted fragment address */
    union {
        struct {
            __le16 l_i_blocks_high; /* were l_i_reserved1 */
            __le16 l_i_file_acl_high;
            __le16 l_i_uid_high;    /* these 2 fields */
            __le16 l_i_gid_high;    /* were reserved2[0] */
            __le16 l_i_checksum_lo; /* crc32c(uuid+inum+inode) LE */
            __le16 l_i_reserved;
        } linux2;
        struct {
            __le16 h_i_reserved1; /* Obsoleted fragment number/size which are removed in ext4 */
            __u16 h_i_mode_high;
            __u16 h_i_uid_high;
            __u16 h_i_gid_high;
            __u32 h_i_author;
        } hurd2;
        struct {
            __le16 h_i_reserved1; /* Obsoleted fragment number/size which are removed in ext4 */
            __le16 m_i_file_acl_high;
            __u32 m_i_reserved2[2];
        } masix2;
    } osd2; /* OS dependent 2 */
    __le16 i_extra_isize;
    __le16 i_checksum_hi;  /* crc32c(uuid+inum+inode) BE */
    __le32 i_ctime_extra;  /* extra Change time      (nsec << 2 | epoch) */
    __le32 i_mtime_extra;  /* extra Modification time(nsec << 2 | epoch) */
    __le32 i_atime_extra;  /* extra Access time      (nsec << 2 | epoch) */
    __le32 i_crtime;       /* File Creation time */
    __le32 i_crtime_extra; /* extra FileCreationtime (nsec << 2 | epoch) */
    __le32 i_version_hi;   /* high 32 bits for 64-bit version */
    __le32 i_projid;       /* Project ID */
};

#define EXT4_INODE_SIZE(inode) (((uint64_t)(inode)->i_size_high << 32) | (inode)->i_size_lo)
#define EXT4_INODE_UID(inode)  ((inode).i_uid | (((uint32_t)(inode).osd2.linux2.l_i_uid_high) << 16))
#define EXT4_INODE_GID(inode)  ((inode).i_gid | (((uint32_t)(inode).osd2.linux2.l_i_gid_high) << 16))
#define EXT4_INODE_SET_UID(inode, uid) \
    ((inode).i_uid = ((uid)&MASK_16), (inode).osd2.linux2.l_i_uid_high = (((uid) >> 16) & MASK_16))
#define EXT4_INODE_SET_GID(inode, gid) \
    ((inode).i_gid = ((gid)&MASK_16), (inode).osd2.linux2.l_i_gid_high = (((gid) >> 16) & MASK_16))

#define EXT4_INODE_SET_SIZE(inode, size) \
    ((inode).i_size_lo = size & MASK_32, (inode).i_size_high = (uint32_t)((((uint64_t)size) >> 32) & MASK_32))
#define EXT4_INODE_SET_BLOCKS(inode, blocks) \
    ((inode).i_blocks_lo = blocks & MASK_32, (inode).osd2.linux2.l_i_blocks_high = (uint32_t)(((uint64_t)blocks) >> 32))

/*
 * Inode flags
 */
#define EXT4_SECRM_FL        0x00000001 /* Secure deletion */
#define EXT4_UNRM_FL         0x00000002 /* Undelete */
#define EXT4_COMPR_FL        0x00000004 /* Compress file */
#define EXT4_SYNC_FL         0x00000008 /* Synchronous updates */
#define EXT4_IMMUTABLE_FL    0x00000010 /* Immutable file */
#define EXT4_APPEND_FL       0x00000020 /* writes to file may only append */
#define EXT4_NODUMP_FL       0x00000040 /* do not dump file */
#define EXT4_NOATIME_FL      0x00000080 /* do not update atime */
/* Reserved for compression usage... */
#define EXT4_DIRTY_FL        0x00000100
#define EXT4_COMPRBLK_FL     0x00000200 /* One or more compressed clusters */
#define EXT4_NOCOMPR_FL      0x00000400 /* Don't compress */
                                        /* nb: was previously EXT2_ECOMPR_FL */
#define EXT4_ENCRYPT_FL      0x00000800 /* encrypted file */
/* End compression flags --- maybe not all used */
#define EXT4_INDEX_FL        0x00001000 /* hash-indexed directory */
#define EXT4_IMAGIC_FL       0x00002000 /* AFS directory */
#define EXT4_JOURNAL_DATA_FL 0x00004000 /* file data should be journaled */
#define EXT4_NOTAIL_FL       0x00008000 /* file tail should not be merged */
#define EXT4_DIRSYNC_FL      0x00010000 /* dirsync behaviour (directories only) */
#define EXT4_TOPDIR_FL       0x00020000 /* Top of directory hierarchies*/
#define EXT4_HUGE_FILE_FL    0x00040000 /* Set to each huge file */
#define EXT4_EXTENTS_FL      0x00080000 /* Inode uses extents */
#define EXT4_VERITY_FL       0x00100000 /* Verity protected inode */
#define EXT4_EA_INODE_FL     0x00200000 /* Inode used for large EA */
/* 0x00400000 was formerly EXT4_EOFBLOCKS_FL */

#define EXT4_DAX_FL          0x02000000 /* Inode is DAX */

#define EXT4_INLINE_DATA_FL  0x10000000 /* Inode has inline data. */
#define EXT4_PROJINHERIT_FL  0x20000000 /* Create with parents projid */
#define EXT4_CASEFOLD_FL     0x40000000 /* Casefolded directory */
#define EXT4_RESERVED_FL     0x80000000 /* reserved for ext4 lib */

/* User modifiable flags */
#define EXT4_FL_USER_MODIFIABLE                                                                                    \
    (EXT4_SECRM_FL | EXT4_UNRM_FL | EXT4_COMPR_FL | EXT4_SYNC_FL | EXT4_IMMUTABLE_FL | EXT4_APPEND_FL |            \
     EXT4_NODUMP_FL | EXT4_NOATIME_FL | EXT4_JOURNAL_DATA_FL | EXT4_NOTAIL_FL | EXT4_DIRSYNC_FL | EXT4_TOPDIR_FL | \
     EXT4_EXTENTS_FL | 0x00400000 /* EXT4_EOFBLOCKS_FL */ | EXT4_DAX_FL | EXT4_PROJINHERIT_FL | EXT4_CASEFOLD_FL)

/* User visible flags */
#define EXT4_FL_USER_VISIBLE                                                                                          \
    (EXT4_FL_USER_MODIFIABLE | EXT4_DIRTY_FL | EXT4_COMPRBLK_FL | EXT4_NOCOMPR_FL | EXT4_ENCRYPT_FL | EXT4_INDEX_FL | \
     EXT4_VERITY_FL | EXT4_INLINE_DATA_FL)

/* Flags that should be inherited by new inodes from their parent. */
#define EXT4_FL_INHERITED                                                                              \
    (EXT4_SECRM_FL | EXT4_UNRM_FL | EXT4_COMPR_FL | EXT4_SYNC_FL | EXT4_NODUMP_FL | EXT4_NOATIME_FL |  \
     EXT4_NOCOMPR_FL | EXT4_JOURNAL_DATA_FL | EXT4_NOTAIL_FL | EXT4_DIRSYNC_FL | EXT4_PROJINHERIT_FL | \
     EXT4_CASEFOLD_FL | EXT4_DAX_FL)

/* Flags that are appropriate for regular files (all but dir-specific ones). */
#define EXT4_REG_FLMASK     (~(EXT4_DIRSYNC_FL | EXT4_TOPDIR_FL | EXT4_CASEFOLD_FL | EXT4_PROJINHERIT_FL))

/* Flags that are appropriate for non-directories/regular files. */
#define EXT4_OTHER_FLMASK   (EXT4_NODUMP_FL | EXT4_NOATIME_FL)

/* The only flags that should be swapped */
#define EXT4_FL_SHOULD_SWAP (EXT4_HUGE_FILE_FL | EXT4_EXTENTS_FL)

/* Flags which are mutually exclusive to DAX */
#define EXT4_DAX_MUT_EXCL   (EXT4_VERITY_FL | EXT4_ENCRYPT_FL | EXT4_JOURNAL_DATA_FL | EXT4_INLINE_DATA_FL)

#endif
