
#include <string.h>
#include <sys/types.h>

#include "bitmap.h"
#include "cache.h"
#include "dentry.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_dentry.h"
#include "ext4/ext4_inode.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

#define RENAME_NOREPLACE 0
#define RENAME_EXCHANGE  1

int mv_in_local_dir(struct ext4_inode *inode, uint32_t inode_idx, const char *from, const char *to) {
    char *oldname = strrchr(from, '/') + 1;
    char *newname = strrchr(to, '/') + 1;
    struct ext4_dir_entry_2 *de_before;
    struct ext4_dir_entry_2 *de = dentry_find(inode, inode_idx, oldname, &de_before);
    ASSERT(de != NULL);

    uint64_t old_len = strlen(oldname);
    uint64_t new_len = strlen(newname);
    uint64_t new_rec_len = DE_CALC_REC_LEN(new_len);

    if (de->rec_len >= new_rec_len || old_len <= new_len) {
        // de is enough, just update inplace
        memcpy(de->name, newname, new_len);
        de->name[new_len] = 0;
        de->name_len = new_len;
        INFO("update dentry %s to %s", oldname, newname);
        dcache_write_back();
        // update decache entry
        struct decache_entry *entry = decache_find(&from);
        if (entry) {
            DEBUG("update decache entry %s to %s", entry->name, newname);
            strcpy(entry->name, newname);
        }
        return 0;
    } else {
        // de name len not enough, need to create a new dentry
        dentry_delete(inode, inode_idx, oldname);
        decache_delete(from);
        ASSERT(0);
        // TODO: allocate a new data block and add a new dentry
    }
    return 0;
}

/** Rename a file
 *
 * *flags* may be `RENAME_EXCHANGE` or `RENAME_NOREPLACE`. If
 * RENAME_NOREPLACE is specified, the filesystem must not
 * overwrite *newname* if it exists and return an error
 * instead. If `RENAME_EXCHANGE` is specified, the filesystem
 * must atomically exchange the two files, i.e. both must
 * exist and neither may be deleted.
 */
int op_rename(const char *from, const char *to, unsigned int flags) {
    DEBUG("rename from %s to %s with flags %d", from, to, flags);

    struct ext4_inode *from_inode_dir, *to_inode_dir;
    uint32_t from_inode_dir_idx, to_inode_dir_idx;

    if (inode_get_parent_by_path(from, &from_inode_dir, &from_inode_dir_idx) < 0) {
        DEBUG("fail to get inode %s dir", from);
        return -ENOENT;
    }
    DEBUG("find from dir inode %d", from_inode_dir_idx);

    if (inode_get_parent_by_path(to, &to_inode_dir, &to_inode_dir_idx) < 0) {
        DEBUG("fail to get inode %s dir", to);
        return -ENOENT;
    }
    DEBUG("find to dir inode %d", to_inode_dir_idx);

    dcache_init(to_inode_dir, to_inode_dir_idx);
    if (dentry_find(to_inode_dir, to_inode_dir_idx, strrchr(to, '/') + 1, NULL) != NULL) {
        WARNING("file %s already exists", to);
        op_unlink(to);
    }

    // FIXME: mv file to dir

    if (from_inode_dir_idx == to_inode_dir_idx) {
        // same dir
        DEBUG("in same dir");
        return mv_in_local_dir(from_inode_dir, from_inode_dir_idx, from, to);
    } else {
        // different dir
        DEBUG("in different dir");
        // dcache_init(from_inode_dir, from_inode_dir_idx);
        // struct ext4_dir_entry_2 *de = dentry_find(from_inode_dir, from_inode_dir_idx, strrchr(from, '/') + 1, NULL);
        // struct ext4_dir_entry_2 *de_last = dentry_last(to_inode_dir, to_inode_dir_idx);
        // ASSERT(de != NULL);

        // dentry_delete(from_inode_dir, from_inode_dir_idx, strrchr(from, '/') + 1);
        // dcache_write_back();
        // DEBUG("delete dentry %s", strrchr(from, '/') + 1);

        // dcache_init(to_inode_dir, to_inode_dir_idx);
        // dentry_create(de_last, strrchr(to, '/') + 1, de->inode_idx, de->file_type);
        // dcache_write_back();
        return 0;
    }
    return 0;
}