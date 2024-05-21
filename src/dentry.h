
#pragma once

#include "ext4/ext4_dentry.h"
#include "inode.h"
#include "logging.h"

#define ALIGN_TO_DENTRY(n) ALIGN_TO(n, 4)

// get the dentry from the directory, with a given offset
struct ext4_dir_entry_2 *dentry_next(struct ext4_inode *inode, uint64_t offset, struct inode_dir_ctx *ctx);

// find the last dentry
struct ext4_dir_entry_2 *dentry_last(struct ext4_inode *inode);

// create a dentry by name and inode_idx
int dentry_create(struct ext4_dir_entry_2 *de, char *name, uint32_t inode_idx);
