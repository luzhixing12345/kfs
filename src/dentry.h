
#pragma once

#include <stdint.h>

#include "ext4/ext4_dentry.h"
#include "inode.h"
#include "logging.h"

#define ALIGN_TO_DENTRY(__n) ALIGN_TO(__n, 4)
#define DE_REAL_REC_LEN(de)  ((__le16)ALIGN_TO_DENTRY((de)->name_len + EXT4_DE_BASE_SIZE))

// get the dentry from the directory, with a given offset
struct ext4_dir_entry_2 *dentry_next(struct ext4_inode *inode, uint64_t offset);

// find the last dentry
struct ext4_dir_entry_2 *dentry_last(struct ext4_inode *inode, uint32_t parent_idx, uint64_t *lblock);

/**
 * @brief create a dentry by name and inode_idx
 *
 * @param de
 * @param name
 * @param inode_idx
 * @return int
 */
int dentry_create(struct ext4_dir_entry_2 *de, char *name, uint32_t inode_idx);

int dentry_add(struct ext4_dir_entry_2 *last_de, struct ext4_dir_entry_2 *de);
