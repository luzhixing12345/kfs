#define FUSE_USE_VERSION 31

#include <errno.h>
#include <fcntl.h>
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include "fs.h"

#define FILE_SYSTEM_ROOT "/"

// 文件系统中的文件列表
static struct file_info files[] = {
    {"file1.txt", S_IFREG | 0644, "Hello, World!", 14},
    {"file2.txt", S_IFREG | 0644, "This is a test file.", 20},
    // 添加更多文件...
    {NULL, 0, NULL, 0} // 列表结束
};


// 初始化文件系统
static void *kfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    // 这里可以进行文件系统的初始化工作
    fprintf(stderr, "kfs init\n");
    cfg->kernel_cache = 1;  // 开启内核缓存
    return NULL;
}

// 清理文件系统
static void kfs_destroy(void *private_data) {
    // 这里可以进行文件系统的清理工作
    printf("kfs destroy\n");
}

// 获取文件信息
static int kfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    struct file_info *file = get_file_info(path + 1, files); // 跳过开头的 '/'
    if (!file) {
        return -ENOENT; // 没有找到文件
    }
    stbuf->st_mode = file->mode;
    stbuf->st_size = file->size;
    return 0;
}

// 读取目录
static int kfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags) {
    // 仅处理根目录
    printf("kfs readdir: %s\n", path);
    (void)offset;
    (void)fi;

    if (strcmp(path, FILE_SYSTEM_ROOT) != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    filler(buf, "file", NULL, 0, 0);
    return 0;
}

// 打开文件
static int kfs_open(const char *path, struct fuse_file_info *fi) {
    struct file_info *file = get_file_info(path + 1, files);
    if (!file) {
        return -ENOENT;
    }
    fi->fh = (uint64_t)file; // 使用文件指针作为文件描述符
    return 0;
}

// FUSE 回调函数:读取文件
static int kfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    struct file_info *file = (struct file_info *)fi->fh;
    size_t file_size = file->size;

    if (offset >= file_size) {
        return 0;  // 读取结束
    }

    size_t read_size = size;
    if (offset + size > file_size) {
        read_size = file_size - offset;
    }

    memcpy(buf, file->data + offset, read_size);
    return read_size;
}

// FUSE 操作结构体
static struct fuse_operations kfs_oper = {
    .init = kfs_init,        // 初始化
    .destroy = kfs_destroy,  // 清理
    .getattr = kfs_getattr,  // 获取文件信息
    .readdir = kfs_readdir,
    .open = kfs_open,
    .read = kfs_read,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &kfs_oper, NULL);
}