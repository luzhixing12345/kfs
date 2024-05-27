/*
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

#include "cache.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_inode.h"
#include "inode.h"
#include "logging.h"

#define DCACHE_ENTRY_CALLOC() calloc(1, sizeof(struct dcache_entry))
#define DCACHE_ENTRY_LIFE     8

/* Locking for the dcache is tricky.  What we try to do is to keep insertions
 * and lookups threadsafe by atomic updates.  That keeps the tree safe, but
 * means that eventually there might be duplicated entries.
 * However, when entry pruning is implemented, there might be a need to take
 * real locks into use.
 *
 * About string handling in this file, it should be said that all strings are
 * provided with a length.  The idea is that usually strings are passed here as
 * they appear on the fuse call (with arbitrary directory depth) but we are
 * just interested on one path token.  Such calculation is done somewhere else,
 * so we just take it from whoever calls us.
 * All this hassle is basically to avoid copying around strings. */

struct dcache_entry root;
struct dcache *dcache;
struct icache *icache;

int cache_init() {
    dcache_init_root(EXT4_ROOT_INO);
    dcache = malloc(sizeof(struct dcache) + BLOCK_SIZE);
    dcache->lblock = -1;
    dcache->pblock = -1;
    dcache->inode_idx = 0;
    INFO("dcache init");

    icache = malloc(sizeof(struct icache));
    icache->entries = calloc(ICACHE_MAX_COUNT, sizeof(struct icache_entry));
    icache->count = 0;
    INFO("icache init");
    return 0;
}

void dcache_update(struct ext4_inode *inode, uint32_t lblock) {
    uint64_t pblock = inode_get_data_pblock(inode, lblock, NULL);
    disk_read_block(pblock, dcache->buf);
    dcache->lblock = lblock;
    dcache->pblock = pblock;
}

/**
 * @brief preload the first logic data block of inode
 *
 * @param inode
 * @param inode_idx
 */
void dcache_init(struct ext4_inode *inode, uint32_t inode_idx) {
    ASSERT(inode_idx != 0);
    ICACHE_UPDATE_CNT(inode);  // update lru count

    // if already initialized
    if (dcache->inode_idx == inode_idx && dcache->lblock == 0) {
        DEBUG("already dcache for inode %u", inode_idx);
        return;
    }

    dcache->inode_idx = inode_idx;
    DEBUG("dctx preload for inode %u", inode_idx);
    dcache_update(inode, 0);
}

int dcache_init_root(uint32_t n) {
    if (root.inode_idx) {
        WARNING("Reinitializing dcache not allowed.  Skipped.");
        return -1;
    }

    /* Root entry doesn't need most of the fields.  Namely, it only uses the
     * inode field and the childs pointer. */
    INFO("Initializing root dcache entry");
    root.inode_idx = n;
    root.childs = NULL;

    return 0;
}

int dcache_write_back() {
    INFO("write back data to disk");
    return disk_write_block(dcache->pblock, dcache->buf);
}

/* Inserts a node as a childs of a given parent.  The parent is updated to
 * point the newly inserted childs as the first childs.  We return the new
 * entry so that further entries can be inserted.
 *
 *      [0]                  [0]
 *       /        ==>          \
 *      /         ==>           \
 * .->[1]->[2]-.       .->[1]->[3]->[2]-.
 * `-----------´       `----------------´
 */
struct dcache_entry *decache_insert(struct dcache_entry *parent, const char *name, int namelen, uint32_t n) {
    /* TODO: Deal with names that exceed the allocated size */
    if (namelen + 1 > DCACHE_ENTRY_NAME_LEN)
        return NULL;

    if (parent == NULL) {
        parent = &root;
        ASSERT(parent->inode_idx);
    }

    struct dcache_entry *new_entry = DCACHE_ENTRY_CALLOC();
    strncpy(new_entry->name, name, namelen);
    new_entry->name[namelen] = 0;
    new_entry->inode_idx = n;

    if (!parent->childs) {
        // add as first child
        DEBUG("parent has no childs, add as first child %s", new_entry->name);
        new_entry->siblings = new_entry;
        parent->childs = new_entry;
    } else {
        DEBUG("parent has childs %s", parent->childs->name);
        new_entry->siblings = parent->childs->siblings;
        parent->childs->siblings = new_entry;
        parent->childs = new_entry;
    }

    return new_entry;
}

// Lookup a cache entry for a given file name.  Return value is a struct pointer
// that can be used to both obtain the inode number and insert further child
// entries.
struct dcache_entry *decache_lookup(struct dcache_entry *parent, const char *name, int namelen) {
    /* TODO: Prune entries by using the LRU counter */
    if (parent == NULL) {
        parent = &root;
    }

    if (!parent->childs) {
        DEBUG("directory has no cached entry");
        return NULL;
    }

    DEBUG("Looking up dcache entry %s:%d", name, namelen);
    /* Iterate the list of siblings to see if there is any match */
    struct dcache_entry *iter = parent->childs;
    do {
        DEBUG("get dcache entry %s", iter->name);
        if (strncmp(iter->name, name, namelen) == 0 && iter->name[namelen] == 0) {
            INFO("match dcache entry %s == %s:%d", iter->name, name, namelen);
            parent->childs = iter;
            return iter;
        }

        iter = iter->siblings;
    } while (iter != parent->childs);

    DEBUG("fail to get cached entry %s", name);
    return NULL;
}

/**
 * @brief find inode in icache
 *
 * @param inode_idx
 * @return struct ext4_inode* NULL if not found
 */
struct ext4_inode *icache_find(uint32_t inode_idx) {
    for (int i = 0; i < icache->count; i++) {
        if (icache->entries[i].status != ICACHE_S_INVAL && icache->entries[i].inode_idx == inode_idx) {
            return &icache->entries[i].inode;
        }
    }
    return NULL;
}

// if has invalid icache entry, then replace it
// otherwise find the least recently used inode, and replace it with the given inode
struct ext4_inode *icache_lru_replace(uint32_t inode_idx, int read_from_disk) {
    int lru_idx = -1;
    int lru_count = INT_MAX;
    uint64_t off;
    for (int i = 0; i < icache->count; i++) {
        // find an invalid entry, repalce it
        if (icache->entries[i].status == ICACHE_S_INVAL) {
            if (read_from_disk) {
                off = inode_get_offset(inode_idx);
                disk_read(off, sizeof(struct ext4_inode), &icache->entries[i].inode);
            }
            icache->entries[i].inode_idx = inode_idx;
            icache->entries[i].status = ICACHE_S_VALID;
            icache->entries[i].lru_count = 0;
            INFO("replace invalid entry %d by inode %d", i, inode_idx + 1);
            return &icache->entries[i].inode;
        }
        // find the least recently used entry
        if (icache->entries[i].lru_count < lru_count) {
            lru_idx = i;
            lru_count = icache->entries[i].lru_count;
        }
    }

    // if the inode is dirty, write it back to disk
    if (icache->entries[lru_idx].status == ICACHE_S_DIRTY) {
        off = inode_get_offset(icache->entries[lru_idx].inode_idx);
        disk_write(off, sizeof(struct ext4_inode), &icache->entries[lru_idx].inode);
    }
    if (read_from_disk) {
        disk_read(inode_get_offset(inode_idx), sizeof(struct ext4_inode), &icache->entries[lru_idx].inode);
    }
    icache->entries[lru_idx].inode_idx = inode_idx;
    icache->entries[lru_idx].status = ICACHE_S_VALID;
    icache->entries[lru_idx].lru_count = 0;
    INFO("replace lru entry %d by inode %d", lru_idx, inode_idx);
    return &icache->entries[lru_idx].inode;
}

/**
 * @brief insert a new inode into icache (LRU if exchange)
 *
 * @param inode_idx
 * @param read_from_disk if false, only register a new inode in i_cache instead of load from disk
    useful when inode_create() is called
 * @return struct ext4_inode*
 */
struct ext4_inode *icache_insert(uint32_t inode_idx, int read_from_disk) {
    if (icache->count == ICACHE_MAX_COUNT) {
        INFO("icache is full");
        return icache_lru_replace(inode_idx, read_from_disk);
    } else {
        icache->entries[icache->count].inode_idx = inode_idx;
        if (read_from_disk) {
            uint64_t off = inode_get_offset(inode_idx);
            disk_read(off, sizeof(struct ext4_inode), &icache->entries[icache->count].inode);
        }
        icache->entries[icache->count].status = ICACHE_S_VALID;
        icache->entries[icache->count].lru_count = 0;
        icache->count++;
        INFO("insert inode %d into icache", inode_idx);
        return &icache->entries[icache->count - 1].inode;
    }
}
