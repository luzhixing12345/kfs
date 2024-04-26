#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define FILE_SYSTEM_ROOT "/myfs"

// 模拟的文件系统结构
struct myfs {
    char *files; // 模拟文件系统中的文件,以字符串形式存储
};

// 初始化文件系统
static void *myfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    // 这里可以进行文件系统的初始化工作
    return NULL;
}

// 清理文件系统
static void myfs_destroy(void *private_data) {
    // 这里可以进行文件系统的清理工作
}

// 获取文件信息
static int myfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    // 填充 stat 结构体
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, FILE_SYSTEM_ROOT) == 0) {
        // 根目录
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (strstr(path, FILE_SYSTEM_ROOT "/file")) {
        // 文件
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen("Hello, World!"); // 假设文件内容为 "Hello, World!"
    } else {
        return -ENOENT; // 路径不存在
    }
    return 0;
}

// 读取目录
static int myfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    // 仅处理根目录
    (void) offset;
    (void) fi;
    if (strcmp(path, FILE_SYSTEM_ROOT) != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    filler(buf, "file", NULL, 0, 0);
    return 0;
}

// 打开文件
static int myfs_open(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path, FILE_SYSTEM_ROOT "/file") != 0)
        return -ENOENT;
    // 允许读取
    if ((fi->flags & 3) == O_RDONLY)
        return 0;
    return -EACCES;
}

// 读取文件
static int myfs_read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi) {
    if (strcmp(path, FILE_SYSTEM_ROOT "/file") != 0)
        return -ENOENT;
    // 假设文件内容为 "Hello, World!"
    const char *content = "Hello, World!";
    int len = strlen(content);
    if (offset >= len) {
        return 0; // 读取完毕
    }
    len -= offset;
    if (len > size) {
        len = size;
    }
    memcpy(buf, content + offset, len);
    return len;
}

// FUSE 操作结构体
static struct fuse_operations myfs_oper = {
    .getattr    = myfs_getattr,
    .readdir    = myfs_readdir,
    .open       = myfs_open,
    .read       = myfs_read,
    .init       = myfs_init,
    .destroy    = myfs_destroy
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &myfs_oper, NULL);
}