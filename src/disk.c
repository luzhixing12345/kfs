/*
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#define _XOPEN_SOURCE 500
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#include "disk.h"
#include "logging.h"

#ifdef __FreeBSD__
#include <string.h>
#endif

static int disk_fd = -1;

static int pread_wrapper(int fd, void *p, size_t size, off_t where) {
#if defined(__FreeBSD__) && !defined(__APPLE__)
#define PREAD_BLOCK_SIZE 1024
    /* FreeBSD needs to read aligned whole blocks.
     * TODO: Check what is a safe block size.
     */
    static __thread uint8_t block[PREAD_BLOCK_SIZE];
    off_t first_offset = where % PREAD_BLOCK_SIZE;
    int ret = 0;

    if (first_offset) {
        /* This is the case if the read doesn't start on a block boundary.
         * We still need to read the whole block and we do, but we only copy to
         * the out pointer the bytes that where actually asked for.  In this
         * case first_offset is the offset into the block. */
        int pread_ret = pread(fd, block, PREAD_BLOCK_SIZE, where - first_offset);
        ASSERT(pread_ret == PREAD_BLOCK_SIZE);

        size_t first_size = MIN(size, (size_t)(PREAD_BLOCK_SIZE - first_offset));
        memcpy(p, block + first_offset, first_size);
        p += first_size;
        size -= first_size;
        where += first_size;
        ret += first_size;

        if (!size)
            return ret;
    }

    ASSERT(where % PREAD_BLOCK_SIZE == 0);

    size_t mid_read_size = (size / PREAD_BLOCK_SIZE) * PREAD_BLOCK_SIZE;
    if (mid_read_size) {
        int pread_ret_mid = pread(fd, p, mid_read_size, where);
        ASSERT((size_t)pread_ret_mid == mid_read_size);

        p += mid_read_size;
        size -= mid_read_size;
        where += mid_read_size;
        ret += mid_read_size;

        if (!size)
            return ret;
    }

    ASSERT(size < PREAD_BLOCK_SIZE);

    int pread_ret_last = pread(fd, block, PREAD_BLOCK_SIZE, where);
    ASSERT(pread_ret_last == PREAD_BLOCK_SIZE);

    memcpy(p, block, size);

    return ret + size;
#else
    return pread(fd, p, size, where);
#endif
}

int disk_open(const char *path) {
    disk_fd = open(path, O_RDWR);
    if (disk_fd < 0) {
        return -errno;
    }

    return 0;
}

int disk_close() {
    return close(disk_fd);
}

uint64_t disk_size() {
    struct stat st;
    if (fstat(disk_fd, &st) < 0) {
        return 0;
    }
    return st.st_size;
}

int __disk_read(off_t where, size_t size, void *p, const char *func, int line) {
    static pthread_mutex_t read_lock = PTHREAD_MUTEX_INITIALIZER;
    ssize_t pread_ret;

    ASSERT(disk_fd >= 0);

    pthread_mutex_lock(&read_lock);
    DEBUG("Disk Read: 0x%jx +0x%zx [%s:%d]", where, size, func, line);
    pread_ret = pread_wrapper(disk_fd, p, size, where);
    pthread_mutex_unlock(&read_lock);
    if (size == 0)
        WARNING("Read operation with 0 size");

    ASSERT((size_t)pread_ret == size);

    return pread_ret;
}

int __disk_write(off_t where, size_t size, void *p, const char *func, int line) {
    static pthread_mutex_t write_lock = PTHREAD_MUTEX_INITIALIZER;
    ssize_t pwrite_ret;

    ASSERT(disk_fd >= 0);

    pthread_mutex_lock(&write_lock);
    DEBUG("Disk Write: 0x%jx +0x%zx [%s:%d]", where, size, func, line);
    pwrite_ret = pwrite(disk_fd, p, size, where);
    pthread_mutex_unlock(&write_lock);

    ASSERT((size_t)pwrite_ret == size);

    return pwrite_ret;
}

int disk_ctx_create(struct disk_ctx *dctx, off_t where, size_t size, uint32_t len) {
    ASSERT(dctx); /* Should be user allocated */
    ASSERT(size);

    dctx->cur = where;
    dctx->size = size * len;
    DEBUG("New disk context: 0x%jx +0x%jx", dctx->cur, dctx->size);

    return 0;
}

int __disk_ctx_read(struct disk_ctx *dctx, size_t size, void *p, const char *func, int line) {
    int ret = 0;

    ASSERT(dctx->size);
    if (dctx->size == 0) {
        WARNING("Using a context with no bytes left");
        return ret;
    }

    /* Truncate if there are too many bytes requested */
    if (size > dctx->size) {
        size = dctx->size;
    }

    ret = __disk_read(dctx->cur, size, p, func, line);
    dctx->size -= ret;
    dctx->cur += ret;

    return ret;
}
