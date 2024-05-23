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

#include "inode.h"
#include "logging.h"
#include "ops.h"

int op_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    DEBUG("getattr(%s)", path);

    struct ext4_inode inode;
    memset(stbuf, 0, sizeof(struct stat));  // clear the struct
    if (inode_get_by_path(path, &inode, NULL) < 0) {
        DEBUG("fail to get inode %s", path);
        return -ENOENT;
    }

    // get umask
    struct fuse_context *cntx = fuse_get_context();
    mode_t umask_val = cntx->umask;

    stbuf->st_mode = inode.i_mode & ~umask_val;
    stbuf->st_nlink = inode.i_links_count;
    stbuf->st_size = EXT4_INODE_SIZE(&inode);
    stbuf->st_blocks = inode.i_blocks_lo;
    stbuf->st_uid = inode.i_uid;
    stbuf->st_gid = inode.i_gid;
    stbuf->st_atime = inode.i_atime;
    stbuf->st_mtime = inode.i_mtime;
    stbuf->st_ctime = inode.i_ctime;

    return 0;
}
