#ifndef DCACHE_H
#define DCACHE_H

#include <linux/limits.h>
#include <stdint.h>

#include "common.h"
#include "ext4/ext4.h"
#include "ext4/ext4_inode.h"

struct dcache_entry;
struct dcache_entry *dcache_insert(struct dcache_entry *parent, const char *name, int namelen, uint32_t n);
struct dcache_entry *dcache_lookup(struct dcache_entry *parent, const char *name, int namelen);
int dcache_init_root(uint32_t n);

#define DCACHE_ENTRY_NAME_LEN NAME_MAX

/* This struct declares a node of a k-tree.  Every node has a pointer to one of
 * the childs and a pointer (in a circular list fashion) to its siblings. */

struct dcache_entry {
    struct dcache_entry *childs;
    struct dcache_entry *siblings;
    uint32_t inode_idx;
    uint16_t lru_count;
    uint8_t user_count;
    char name[DCACHE_ENTRY_NAME_LEN];
};

// inode directory cache
struct dcache {
    uint32_t inode_idx;  // current inode_idx
    uint32_t lblock;     // current logic block id
    uint8_t buf[];       // buffer
};

int cache_init();

/**
 * @brief preload the first logic data block of inode
 *
 * @param inode
 * @param inode_idx
 */
void dcache_init(struct ext4_inode *inode, uint32_t inode_idx);
void dcache_update(struct ext4_inode *inode, uint32_t lblock);

#define ICACHE_MAX_COUNT 64

// TODO: hashmap
struct icache {
    struct icache_entry *entries;
    uint32_t count;
};

struct icache_entry {
    struct ext4_inode inode;  // inode cached
    uint32_t inode_idx;       // inode index
    uint32_t lru_count;       // lru count
    int valid;                // whether the inode is valid, if modified then set to 0
};

/**
 * @brief find inode in icache
 *
 * @param inode_idx
 * @return struct ext4_inode* NULL if not found
 */
struct ext4_inode *icache_find(uint32_t inode_idx);

/**
 * @brief insert a new inode into icache (LRU if exchange)
 *
 * @param inode_idx
 * @param inode
 * @return void
 */
void icache_insert(uint32_t inode_idx, struct ext4_inode *inode);

/**
 * @brief update inode data
 *
 * @param inode_idx
 * @param inode
 * @return int
 */
int icache_update(uint32_t inode_idx, struct ext4_inode *inode);

#endif
