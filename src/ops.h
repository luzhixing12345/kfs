
#pragma once

#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>

void *op_init(struct fuse_conn_info *info, struct fuse_config *cfg);
int op_readlink(const char *path, char *buf, size_t bufsize);
int op_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int op_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi,
               enum fuse_readdir_flags flags);
int op_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);
int op_open(const char *path, struct fuse_file_info *fi);
void op_destory(void *data);
int op_access(const char *path, int mask);