/*
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 *
 * Obviously inspired on the great FUSE hello world example:
 *      - http://fuse.sourceforge.net/helloworld.html
 */

#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

#ifndef EXT4FUSE_VERSION
#define EXT4FUSE_VERSION "v0.0.1"
#endif

static struct fuse_operations e4f_ops = {.init = op_init,
                                         .getattr = op_getattr,
                                         .access = op_access,
                                         .readdir = op_readdir,
                                         // .releasedir = op_releasedir,
                                         .readlink = op_readlink,
                                         // .mknod = op_mknod,
                                         .mkdir = op_mkdir,
                                         .link = op_link,
                                         .symlink = op_symlink,
                                         .unlink = op_unlink,
                                         .rmdir = op_rmdir,
                                         .rename = op_rename,
                                         .chmod = op_chmod,
                                         .chown = op_chown,
                                         // .truncate = op_truncate,
                                         .utimens = op_utimens,
                                         .open = op_open,
                                         .flush = op_flush,
                                         // .fsync = op_fsync,
                                         // .release = op_release,
                                         .read = op_read,
                                         .write = op_write,
                                         // .statfs = op_statfs,
                                         .create = op_create,
                                         .destroy = op_destory,
                                         .statfs = op_statfs,
                                         .lock = op_lock,
                                         .lseek = op_lseek};

static struct e4f {
    char *disk;
    char *logfile;
} e4f;

static struct fuse_opt e4f_opts[] = {{"logfile=%s", offsetof(struct e4f, logfile), 0}, FUSE_OPT_END};

static int e4f_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs) {
    (void)data;
    (void)outargs;

    switch (key) {
        case FUSE_OPT_KEY_OPT:
            return 1;
        case FUSE_OPT_KEY_NONOPT:
            if (!e4f.disk) {
                e4f.disk = strdup(arg);
                return 0;
            }
            return 1;
        default:
            fprintf(stderr, "internal error\n");
            abort();
    }
}

void signal_handle_sigsegv(int signal) {
    UNUSED(signal);
    void *array[10];
    size_t size;
    char **strings;
    size_t i;

    DEBUG("========================================");
    DEBUG("Segmentation Fault.  Starting backtrace:");
    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);

    for (i = 0; i < size; i++) DEBUG("%s", strings[i]);
    DEBUG("========================================");

    abort();
}

int main(int argc, char *argv[]) {
    int res;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    if (signal(SIGSEGV, signal_handle_sigsegv) == SIG_ERR) {
        fprintf(stderr, "Failed to initialize signals\n");
        return EXIT_FAILURE;
    }

    // Default options
    e4f.disk = NULL;
    e4f.logfile = DEFAULT_LOG_FILE;

    if (fuse_opt_parse(&args, &e4f, e4f_opts, e4f_opt_proc) == -1) {
        return EXIT_FAILURE;
    }

    if (!e4f.disk) {
        fprintf(stderr, "Version: %s\n", EXT4FUSE_VERSION);
        fprintf(stderr, "Usage: %s <disk> <mountpoint>\n", argv[0]);
        exit(1);
    }

    if (logging_open(e4f.logfile) < 0) {
        fprintf(stderr, "Failed to initialize logging\n");
        return EXIT_FAILURE;
    }

    if (disk_open(e4f.disk) < 0) {
        fprintf(stderr, "disk_open: %s: %s\n", e4f.disk, strerror(errno));
        return EXIT_FAILURE;
    }

    off_t disk_magic_offset = BOOT_SECTOR_SIZE + offsetof(struct ext4_super_block, s_magic);
    uint16_t disk_magic;
    if (disk_read(disk_magic_offset, sizeof(disk_magic), &disk_magic) < 0) {
        fprintf(stderr, "Failed to read disk: %s\n", e4f.disk);
        return EXIT_FAILURE;
    }

    // Check if the disk is an ext4 filesystem
    if (disk_magic != EXT4_S_MAGIC) {
        fprintf(stderr, "Magic number mismatch, partition doesn't contain EXT4 filesystem\n");
        return EXIT_FAILURE;
    }

    res = fuse_main(args.argc, args.argv, &e4f_ops, NULL);

    fuse_opt_free_args(&args);
    free(e4f.disk);

    return res;
}
