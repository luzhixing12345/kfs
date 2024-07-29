
#pragma once

#include <stdint.h>

#include "ext4/ext4_dentry.h"
#include "inode.h"
#include "logging.h"

#define ALIGN_TO_DENTRY(__n)  ALIGN_TO(__n, 4)
#define DE_REAL_REC_LEN(de)   ((__le16)ALIGN_TO_DENTRY((de)->name_len + EXT4_DE_BASE_SIZE))
#define DE_CALC_REC_LEN(size) (ALIGN_TO_DENTRY(EXT4_DE_BASE_SIZE + (size)))

// get the dentry from the directory, with a given offset
struct ext4_dir_entry_2 *dentry_next(struct ext4_inode *inode, uint32_t inode_idx, uint64_t offset);

// find the last dentry
struct ext4_dir_entry_2 *dentry_last(struct ext4_inode *inode, uint32_t inode_idx);

/**
 * @brief create a dentry by name and inode_idx
 *
 * @param de
 * @param name
 * @param inode_idx
 * @return int
 */
struct ext4_dir_entry_2 *dentry_create(struct ext4_dir_entry_2 *de, char *name, uint32_t inode_idx, int file_type);

int dentry_add(struct ext4_inode *inode, uint32_t dir_inode_idx, uint32_t inode_idx, char *name);

int dentry_init(uint32_t parent_idx, uint32_t inode_idx);

int dentry_delete(struct ext4_inode *inode, uint32_t inode_idx, char *name);

/**
 * @brief whether the dentry has enough space for a new dentry
 *
 * @param de
 * @param name_len
 * @return int
 */
int dentry_has_enough_space(struct ext4_dir_entry_2 *de, uint64_t name_len);

/**
 * @brief find dentry by name
 *
 * @param inode
 * @param inode_idx
 * @param name
 * @param de_before if not NULL, set the dentry before the found one
 * @return struct ext4_dir_entry_2*
 */
struct ext4_dir_entry_2 *dentry_find(struct ext4_inode *inode, uint32_t inode_idx, char *name,
                                     struct ext4_dir_entry_2 **de_before);