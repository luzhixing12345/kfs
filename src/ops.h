
#pragma once
#include "common.h"
#include "ext4/ext4.h"

void *op_init(struct fuse_conn_info *info, struct fuse_config *cfg);
int op_readlink(const char *path, char *buf, size_t bufsize);
int op_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int op_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi,
               enum fuse_readdir_flags flags);
int op_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);
int op_open(const char *path, struct fuse_file_info *fi);
void op_destory(void *data);
int op_access(const char *path, int mask);
int op_flush(const char *path, struct fuse_file_info *fi);
int op_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int op_utimens(const char *path, const struct timespec ts[2], struct fuse_file_info *fi);
int op_mknod(const char *path, mode_t mode, dev_t dev);
int op_mkdir(const char *path, mode_t mode);
int op_unlink(const char *path);
int op_rmdir(const char *path);
int op_symlink(const char *from, const char *to);
int op_rename(const char *, const char *, unsigned int flags);
int op_release(const char *path, struct fuse_file_info *fi);
int op_link(const char *from, const char *to);
int op_chmod(const char *, mode_t, struct fuse_file_info *fi);
int op_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi);
int op_truncate(const char *path, off_t size, struct fuse_file_info *fi);
int op_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int op_releasedir(const char *path, struct fuse_file_info *fi);
int op_statfs(const char *path, struct statvfs *stbuf);
int op_fsync(const char *path, int isdatasync, struct fuse_file_info *fi);
int op_lock(const char *path, struct fuse_file_info *fi, int cmd, struct flock *lock);
off_t op_lseek(const char *, off_t off, int whence, struct fuse_file_info *);

// in op_unlink.c
int unlink_inode(struct ext4_inode *inode, uint32_t inode_idx);
// in ctl.c
int sb_status(char *buf);