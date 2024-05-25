/*
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "cache.h"
#include "ext4/ext4.h"
#include "ext4/ext4_inode.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

int op_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    DEBUG("getattr(%s)", path);

    struct ext4_inode *inode;
    memset(stbuf, 0, sizeof(struct stat));  // clear the struct
    if (inode_get_by_path(path, &inode, NULL) < 0) {
        DEBUG("fail to get inode %s", path);
        return -ENOENT;
    }

    // get umask
    struct fuse_context *cntx = fuse_get_context();
    mode_t umask_val = cntx->umask;

    stbuf->st_mode = inode->i_mode & ~umask_val;
    stbuf->st_nlink = inode->i_links_count;
    stbuf->st_size = EXT4_INODE_SIZE(inode);

    // Lower 32-bits of "block" count. If the huge_file feature flag is not set on the filesystem, the file consumes
    // i_blocks_lo 512-byte blocks on disk. If huge_file is set and EXT4_HUGE_FILE_FL is NOT set in inode.i_flags, then
    // the file consumes i_blocks_lo + (i_blocks_hi << 32) 512-byte blocks on disk. If huge_file is set and
    // EXT4_HUGE_FILE_FL IS set in inode.i_flags, then this file consumes (i_blocks_lo + i_blocks_hi << 32) filesystem
    // blocks on disk.
    __blkcnt_t blocks = 0;
    if (!(sb.s_flags & EXT4_HUGE_FILE_FL)) {
        blocks = inode->i_blocks_lo;
    } else {
        blocks = inode->i_blocks_lo + (((uint64_t)inode->osd2.linux2.l_i_blocks_high) << 32);
    }
    stbuf->st_blocks = blocks;
    stbuf->st_uid = EXT4_INODE_UID(inode);
    stbuf->st_gid = EXT4_INODE_GID(inode);
    stbuf->st_atime = inode->i_atime;
    stbuf->st_mtime = inode->i_mtime;
    stbuf->st_ctime = inode->i_ctime;

    ICACHE_UPDATE_CNT(inode);
    return 0;
}
