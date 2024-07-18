#ifndef DCACHE_H
#define DCACHE_H

#include <linux/limits.h>
#include <stdint.h>

#include "common.h"
#include "ext4/ext4.h"
#include "ext4/ext4_dentry.h"
#include "ext4/ext4_inode.h"

struct decache_entry;
struct decache_entry *decache_insert(struct decache_entry *parent, const char *name, int namelen, uint32_t n);
struct decache_entry *decache_walk(struct decache_entry *parent, const char *name, int namelen);
int decache_init_root(uint32_t n);
void decache_free(struct decache_entry *entry);
struct decache_entry *decache_find(const char **path);
int decache_delete(const char *path);

#define DCACHE_ENTRY_NAME_LEN NAME_MAX

/* This struct declares a node of a k-tree.  Every node has a pointer to one of
 * the childs and a pointer (in a circular list fashion) to its siblings. */

struct decache_entry {
    struct decache_entry *parent;
    struct decache_entry *childs;
    struct decache_entry *last_child;
    struct decache_entry *prev;
    struct decache_entry *next;
    uint32_t inode_idx;
    uint32_t count;
    char name[DCACHE_ENTRY_NAME_LEN + 1];
};

#define DCACHE_MAX_CHILDREN 20  // each directory can have at most 20 children

// inode directory cache
struct dcache {
    uint32_t inode_idx;  // current inode_idx
    uint32_t lblock;     // current logic block id
    uint32_t pblock;     // current physical block id, for quick write back
    bool dirty;          // dirty flag
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
    struct ext4_inode inode;           // inode cached
    uint32_t inode_idx;                // inode index
    uint32_t lru_count;                // lru count
    int status;                        // empty, valid, dirty
    struct ext4_dir_entry_2 *last_de;  // last dentry offset(only for dir)
                                       // used for quick dentry_last()
};

#define ICACHE_S_INVAL 0
#define ICACHE_S_VALID 1
#define ICACHE_S_DIRTY 2

#define ICACHE_LRU_INC(inode)                                       \
    do {                                                            \
        if (((struct icache_entry *)inode)->lru_count < UINT32_MAX) \
            (((struct icache_entry *)inode)->lru_count++);          \
    } while (0)

#define ICACHE_SET_INVAL(inode)       (((struct icache_entry *)(inode))->status = ICACHE_S_INVAL)
#define ICACHE_SET_DIRTY(inode)       (((struct icache_entry *)(inode))->status = ICACHE_S_DIRTY)
#define ICACHE_IS_DIRTY(inode)        (((struct icache_entry *)(inode))->status == ICACHE_S_DIRTY)
#define ICACHE_IS_VALID(inode)        (((struct icache_entry *)(inode))->status != ICACHE_S_INVAL)
// last dentry offset macro
#define ICACHE_CHECK_LAST_DE(inode)   (ICACHE_IS_VALID(inode) && ((struct icache_entry *)(inode))->last_de != NULL)
#define ICACHE_GET_LAST_DE(inode)     (((struct icache_entry *)(inode))->last_de)
#define ICACHE_SET_LAST_DE(inode, de) (((struct icache_entry *)(inode))->last_de = (de))

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

void icache_write_back(struct icache_entry *entry);

#endif
