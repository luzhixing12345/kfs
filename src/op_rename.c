
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

#define RENAME_NOREPLACE 0  // to is exist
#define RENAME_EXCHANGE  1  // to is not exist

int mv_in_local_dir(struct ext4_inode *inode, uint32_t inode_idx, const char *from, const char *to, int flags) {
    char *oldname = strrchr(from, '/') + 1;
    char *newname = strrchr(to, '/') + 1;
    struct ext4_dir_entry_2 *from_de = dentry_find(inode, inode_idx, oldname, NULL);
    ASSERT(from_de != NULL);

    if (flags == RENAME_EXCHANGE) {
        uint64_t old_len = strlen(oldname);
        uint64_t new_len = strlen(newname);
        uint64_t new_rec_len = DE_CALC_REC_LEN(new_len);
        DEBUG("%s dentry doesn't exist", newname);
        // newname dentry doesn't exist
        if (from_de->rec_len >= new_rec_len || old_len <= new_len) {
            // de is enough, just update inplace
            memcpy(from_de->name, newname, new_len);
            from_de->name[new_len] = 0;
            from_de->name_len = new_len;
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
    } else {
        // RENAME_NOREPLACE
        struct ext4_inode *to_inode;
        uint32_t to_inode_idx;
        if (inode_get_by_path(to, &to_inode, &to_inode_idx) < 0) {
            DEBUG("fail to get inode %s", to);
            return -ENOENT;
        }
        DEBUG("%s dentry already exists", newname);
        unlink_inode(to_inode, to_inode_idx);
        DEBUG("unlink old to dentry %s[%d]", newname, to_inode_idx);

        struct ext4_dir_entry_2 *to_de = dentry_find(inode, inode_idx, newname, NULL);
        ASSERT(to_de != NULL);
        to_de->inode_idx = from_de->inode_idx;
        to_de->file_type = from_de->file_type;
        struct decache_entry *entry = decache_find(&to);
        if (entry) {
            DEBUG("update inode_idx %d to %d in decache entry %s", to_inode_idx, from_de->inode_idx, entry->name);
            entry->inode_idx = from_de->inode_idx;
        }
        INFO("update dentry %s to %s", oldname, newname);

        decache_delete(from);
        dentry_delete(inode, inode_idx, oldname);

        dcache_write_back();
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

    // find from's dir
    if (inode_get_parent_by_path(from, &from_inode_dir, &from_inode_dir_idx) < 0) {
        DEBUG("fail to get inode %s dir", from);
        return -ENOENT;
    }
    DEBUG("find from dir inode %d", from_inode_dir_idx);
    // delete from's dentry
    char *from_filename = strrchr(from, '/') + 1;
    struct ext4_dir_entry_2 *from_de = dentry_find(from_inode_dir, from_inode_dir_idx, from_filename, NULL);
    ASSERT(from_de != NULL);
    uint32_t from_inode_idx = from_de->inode_idx;
    uint32_t from_file_type = from_de->file_type;
    dentry_delete(from_inode_dir, from_inode_dir_idx, from_filename);
    dcache_write_back();
    DEBUG("delete dentry %s", from_filename);
    decache_delete(from);

    // find to's dir
    if (inode_get_parent_by_path(to, &to_inode_dir, &to_inode_dir_idx) < 0) {
        DEBUG("fail to get inode %s dir", to);
        return -ENOENT;
    }
    DEBUG("find to dir inode %d", to_inode_dir_idx);

    char *to_filename = strrchr(to, '/') + 1;
    if (flags == RENAME_EXCHANGE) {
        // to dentry doesn't exist
        struct ext4_dir_entry_2 *last_de = dentry_last(to_inode_dir, to_inode_dir_idx);
        struct ext4_dir_entry_2 *new_de = dentry_create(last_de, to_filename, from_inode_idx, from_file_type);
        ICACHE_SET_LAST_DE(to_inode_dir, new_de);
        INFO("create dentry %s", to_filename);
        dcache_write_back();
    } else {
        // RENAME_NOREPLACE
        // to dentry exists, if to is a dir, unlink original to dentry; if to is a dir, mov to to's dir
        struct ext4_dir_entry_2 *to_de = dentry_find(to_inode_dir, to_inode_dir_idx, to_filename, NULL);
        ASSERT(to_de != NULL);

        // unlink original to dentry
        struct ext4_inode *to_inode;
        uint32_t to_inode_idx;
        if (inode_get_by_path(to, &to_inode, &to_inode_idx) < 0) {
            DEBUG("fail to get inode %s", to);
            return -ENOENT;
        }
        DEBUG("unlink old to dentry %s", to_filename);
        unlink_inode(to_inode, to_inode_idx);
        DEBUG("change to_de from %d to %d", to_de->inode_idx, from_inode_idx);
        to_de->inode_idx = from_inode_idx;
        to_de->file_type = from_file_type;
        struct decache_entry *entry = decache_find(&to);
        if (entry) {
            DEBUG("update inode_idx %d to %d in decache entry %s", to_inode_idx, from_inode_idx, entry->name);
            entry->inode_idx = from_inode_idx;
        }
        dentry_delete(to_inode_dir, to_inode_dir_idx, to_filename);
        dcache_write_back();
        // don't need to create new dentry or delete decache because to_de is already exists
    }
    return 0;
}