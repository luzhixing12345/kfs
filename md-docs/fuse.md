
# fuse

```c
struct fuse_operations {
    // 获取文件属性,类似于 stat() 系统调用
    int (*getattr) (const char *, struct stat *);

    // 读取符号链接的目标路径
    int (*readlink) (const char *, char *, size_t);

    // 已弃用,建议使用 readdir() 替代
    int (*getdir) (const char *, fuse_dirh_t, fuse_dirfil_t);

    // 创建文件节点,用于创建所有非目录、非符号链接节点
    int (*mknod) (const char *, mode_t, dev_t);

    // 创建目录
    int (*mkdir) (const char *, mode_t);

    // 删除文件
    int (*unlink) (const char *);

    // 删除目录
    int (*rmdir) (const char *);

    // 创建符号链接
    int (*symlink) (const char *, const char *);

    // 重命名文件
    int (*rename) (const char *, const char *);

    // 创建文件的硬链接
    int (*link) (const char *, const char *);

    // 更改文件的权限位
    int (*chmod) (const char *, mode_t);

    // 更改文件的所有者和组
    int (*chown) (const char *, uid_t, gid_t);

    // 更改文件的大小
    int (*truncate) (const char *, off_t);

    // 更改文件的访问和/或修改时间,已弃用,建议使用 utimens() 替代
    int (*utime) (const char *, struct utimbuf *);

    // 文件打开操作
    int (*open) (const char *, struct fuse_file_info *);

    // 从打开的文件中读取数据
    int (*read) (const char *, char *, size_t, off_t, struct fuse_file_info *);

    // 向打开的文件写入数据
    int (*write) (const char *, const char *, size_t, off_t, struct fuse_file_info *);

    // 获取文件系统统计信息
    int (*statfs) (const char *, struct statvfs *);

    // 可能刷新缓存数据,并非等同于 fsync()
    int (*flush) (const char *, struct fuse_file_info *);

    // 释放打开的文件
    int (*release) (const char *, struct fuse_file_info *);

    // 同步文件内容
    int (*fsync) (const char *, int, struct fuse_file_info *);

    // 设置扩展属性
    int (*setxattr) (const char *, const char *, const char *, size_t, int);

    // 获取扩展属性
    int (*getxattr) (const char *, const char *, char *, size_t);

    // 列出扩展属性
    int (*listxattr) (const char *, char *, size_t);

    // 删除扩展属性
    int (*removexattr) (const char *, const char *);

    // 打开目录
    int (*opendir) (const char *, struct fuse_file_info *);

    // 读取目录
    int (*readdir) (const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *);

    // 释放目录
    int (*releasedir) (const char *, struct fuse_file_info *);

    // 同步目录内容
    int (*fsyncdir) (const char *, int, struct fuse_file_info *);

    // 初始化文件系统
    void *(*init) (struct fuse_conn_info *);

    // 清理文件系统
    void (*destroy) (void *);

    // 检查文件访问权限
    int (*access) (const char *, int);

    // 创建并打开文件
    int (*create) (const char *, mode_t, struct fuse_file_info *);

    // 更改打开文件的大小
    int (*ftruncate) (const char *, off_t, struct fuse_file_info *);

    // 从打开的文件获取属性
    int (*fgetattr) (const char *, struct stat *, struct fuse_file_info *);

    // 执行 POSIX 文件锁定操作
    int (*lock) (const char *, struct fuse_file_info *, int cmd, struct flock *);

    // 更改文件的访问和修改时间,具有纳秒级分辨率
    int (*utimens) (const char *, const struct timespec tv[2]);

    // 映射文件内块索引到设备内块索引
    int (*bmap) (const char *, size_t blocksize, uint64_t *idx);

    // 标志位,指示文件系统可以接受 NULL 路径作为某些操作的第一个参数
    unsigned int flag_nullpath_ok:1;

    // 标志位,指示某些操作不需要计算路径
    unsigned int flag_nopath:1;

    // 标志位,指示文件系统接受在其 utimens 操作中的 UTIME_NOW 和 UTIME_OMIT 特殊值
    unsigned int flag_utime_omit_ok:1;

    // 保留的标志位,不要设置
    unsigned int flag_reserved:29;

    // Ioctl 操作
    int (*ioctl) (const char *, int cmd, void *arg, struct fuse_file_info *, unsigned int flags, void *data);

    // 轮询 IO 就绪事件
    int (*poll) (const char *, struct fuse_file_info *, struct fuse_pollhandle *ph, unsigned *reventsp);

    // 将缓冲区的内容写入打开的文件
    int (*write_buf) (const char *, struct fuse_bufvec *buf, off_t off, struct fuse_file_info *);

    // 将打开的文件的数据存储在缓冲区中
    int (*read_buf) (const char *, struct fuse_bufvec **bufp, size_t size, off_t off, struct fuse_file_info *);

    // 执行 BSD 文件锁定操作
    int (*flock) (const char *, struct fuse_file_info *, int op);

    // 为打开的文件分配空间
    int (*fallocate) (const char *, int, off_t, off_t, struct fuse_file_info *);
};
```

LOOPUP (getattr)

CREATE (create)

(getattr)

FLUSH (flush)

SETATTR (utimens)

getattr[NULL]

## 参考

- [libfuse](https://github.com/libfuse/libfuse)
- [5分钟搞懂用户空间文件系统FUSE工作原理](https://zhuanlan.zhihu.com/p/106719192)
- [FUSE的使用及示例](https://zhoubofsy.github.io/2017/01/13/linux/filesystem-userspace-usage/)
- [u_fs](https://github.com/Tan-Cc/u_fs)
- [ext4fuse](https://github.com/gerard/ext4fuse)
- [fuse-ext2](https://github.com/alperakcan/fuse-ext2)
- [FAT 和 UNIX 文件系统 (磁盘上的数据结构) [南京大学2023操作系统-P28] (蒋炎岩)](https://www.bilibili.com/video/BV1xN411C74V/)
- [14.ext2文件系统](https://www.bilibili.com/video/BV1V84y1A7or/)
- [UserSpace-Memory-Fuse-System](https://github.com/jinCode-gao/UserSpace-Memory-Fuse-System)