/*
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

#include <stdlib.h>
#include <string.h>

#include "dcache.h"
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
struct dcache_entry *dcache_insert(struct dcache_entry *parent, const char *name, int namelen, uint32_t n) {
    DEBUG("Inserting %s,%d to dcache", name, namelen);

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
        DEBUG("parent has no childs");
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

/**
 * @brief Lookup a cache entry for a given file name.  Return value is a struct pointer
 * that can be used to both obtain the inode number and insert further child
 * entries.
 *
 * @param parent
 * @param name
 * @param namelen
 * @return struct dcache_entry* if found, NULL otherwise
 */
struct dcache_entry *dcache_lookup(struct dcache_entry *parent, const char *name, int namelen) {
    /* TODO: Prune entries by using the LRU counter */
    if (parent == NULL) {
        parent = &root;
    }

    if (!parent->childs) {
        DEBUG("Looking up %s: Not found (no childs)", name);
        return NULL;
    }

    /* Iterate the list of siblings to see if there is any match */
    struct dcache_entry *iter = parent->childs;
    do {
        if (strncmp(iter->name, name, namelen) == 0 && iter->name[namelen] == 0) {
            parent->childs = iter;
            INFO("get dcache entry %s", name);
            return iter;
        }

        iter = iter->siblings;
    } while (iter != parent->childs);

    DEBUG("fail to get cached entry %s", name);
    return NULL;
}
