/*
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include "common.h"
#include "ext4/ext4_inode.h"
#include "inode.h"
#include "logging.h"
#include "common.h"



int op_open(const char *path, struct fuse_file_info *fi) {
    DEBUG("open %s with flags %o", path, fi->flags);

    uint32_t inode_idx = inode_get_idx_by_path(path);
    struct ext4_inode *inode;
    if (inode_get_by_number(inode_idx, &inode) < 0) {
        DEBUG("fail to get inode %d", inode_idx);
        return -ENOENT;
    }

    access_mode_t mode = fi->flags & O_ACCMODE;
    // check if user has read permission
    if (inode_check_permission(inode, mode) < 0) {
        DEBUG("fail to check permission for inode %d", inode_idx);
        return -EACCES;
    }

    fi->fh = inode_idx;
    DEBUG("%s is inode %d", path, fi->fh);

    return 0;
}
