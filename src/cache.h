#ifndef DCACHE_H
#define DCACHE_H

#include <linux/limits.h>
#include <stdint.h>

#include "common.h"
#include "ext4/ext4.h"
#include "ext4/ext4_inode.h"

struct dcache_entry;
struct dcache_entry *decache_insert(struct dcache_entry *parent, const char *name, int namelen, uint32_t n);
struct dcache_entry *decache_lookup(struct dcache_entry *parent, const char *name, int namelen);
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
    uint32_t pblock;     // current physical block id, for quick write back
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
int dcache_write_back();

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
    int status;               // empty, valid, dirty
};

#define ICACHE_S_INVAL           0
#define ICACHE_S_VALID           1
#define ICACHE_S_DIRTY           2

#define ICACHE_UPDATE_CNT(inode) (((struct icache_entry *)(inode))->lru_count++)
#define ICACHE_INVAL(inode)      (((struct icache_entry *)(inode))->status = ICACHE_S_INVAL)
#define ICACHE_DIRTY(inode)      (((struct icache_entry *)(inode))->status = ICACHE_S_DIRTY)

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
 * @param read_from_disk if false, only register a new inode in i_cache instead of load from disk
    useful when inode_create() is called
 * @return struct ext4_inode*
 */
struct ext4_inode *icache_insert(uint32_t inode_idx, int read_from_disk);

#endif
