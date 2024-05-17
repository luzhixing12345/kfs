/*
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

#include <stdlib.h>

#include "inode.h"
#include "logging.h"
#include "ops.h"
#include "super.h"

void *op_init(struct fuse_conn_info *info, struct fuse_config *cfg) {
    INFO("Using FUSE protocol %d.%d", info->proto_major, info->proto_minor);
    cfg->kernel_cache = 1;  // Enable kernel cache

    // Initialize the super block
    super_fill();        // superblock
    super_group_fill();  // group descriptors
    inode_init();        // root inode

    return NULL;
}
