#ifndef INODE_H
#define INODE_H

#include <stdint.h>
#include <sys/types.h>

#include "ext4/ext4_dentry.h"
#include "ext4/ext4_inode.h"

// inode directory context
struct inode_dir_ctx {
    uint32_t lblock; /* Currently buffered lblock */
    uint8_t buf[];
};

uint64_t inode_get_data_pblock(struct ext4_inode *inode, uint32_t lblock, uint32_t *extent_len);

struct inode_dir_ctx *inode_dir_ctx_get(void);
void inode_dir_ctx_put(struct inode_dir_ctx *);
void inode_dir_ctx_reset(struct inode_dir_ctx *ctx, struct ext4_inode *inode);
struct ext4_dir_entry_2 *inode_dentry_get(struct ext4_inode *inode, uint64_t offset, struct inode_dir_ctx *ctx);

int inode_get_by_number(uint32_t n, struct ext4_inode *inode);
int inode_get_by_path(const char *path, struct ext4_inode *inode);
uint32_t inode_get_idx_by_path(const char *path);

int inode_init(void);

#endif