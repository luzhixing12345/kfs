#ifndef DCACHE_H
#define DCACHE_H

#include <linux/limits.h>
#include <stdint.h>

#include "common.h"

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

int dcache_init();

#endif
