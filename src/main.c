/*
UserSpace-Memory-Fuse-System, developed using libfuse.
*/

#define FUSE_USE_VERSION 31
#define MAX_NODES 100
#define MAX_CHILDREN 50

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse3/fuse.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "fs.h"

struct node *nodes[MAX_NODES];  // list of node

int file_num = 0;  // numbers of files and directory

// get filename from path
char *get_file(const char *path) {
    print_info("GetFileNamefromPath is called\n");
    char *filename_temp = strrchr(path, '/');
    // printf("filename: %s\n", filename_temp);
    return strdup(filename_temp + 1);
}

// Get the path of the parent node
const char *find_parent_dir(const char *path) {
    char *last_slash = strrchr(path, '/');

    // If the parent directory is the root directory
    if (last_slash == path)
        return "/";

    char *parent_path = (char *)malloc(last_slash - path + 1);
    strncpy(parent_path, path, last_slash - path);
    parent_path[last_slash - path] = '\0';
    return parent_path;
}

// Create a node.
struct node *create_node(const char *path, mode_t mode, struct node *parent) {
    print_info("CreateNode is called\n");
    struct node *new_node = (struct node *)malloc(sizeof(struct node));
    if (strcmp(path, "/"))
        new_node->filename = get_file(path);
    else
        new_node->filename = "";
    new_node->path = strdup(path);
    new_node->contents = NULL;

    new_node->mode = mode;

    new_node->parent = parent;
    new_node->child_count = 0;

    // set time
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);

    print_info("see time:%d-%d-%d %d:%d:%d\n",
               timeinfo->tm_year + 1900,
               timeinfo->tm_mon + 1,
               timeinfo->tm_mday,
               timeinfo->tm_hour,
               timeinfo->tm_min,
               timeinfo->tm_sec);

    print_info("mktime result: %ld\n", mktime(timeinfo));
    new_node->atime = mktime(timeinfo);
    new_node->mtime = mktime(timeinfo);
    new_node->ctime = mktime(timeinfo);

    nodes[file_num] = new_node;
    file_num++;
    if ((mode & S_IFMT) == S_IFDIR)
        new_node->children = (struct node **)malloc(MAX_CHILDREN * sizeof(struct node *));  // file has no children
    else
        new_node->children = NULL;

    return new_node;
}

// Print information of a node for debugging
void print_node(struct node *node) {
    if (node == NULL) {
        printf("Error: node is NULL\n");
        return;
    }
    print_info("PrintNode is called\n");
    if (node->filename != NULL)
        printf("filename: %s\n", node->filename);
    else
        printf("filename: \n");
    if (node->contents != NULL)
        printf("contents: %s\n", node->contents);
    else
        printf("contents: \n");
    if (node->path != NULL)
        printf("path: %s\n", node->path);
    else
        printf("path: \n");
    printf("child_count: %d\n", node->child_count);
    if (node->mode)
        printf("mode: %d\n", node->mode);
    else
        printf("mode: \n");
    if (node->child_count != 0) {
        for (int i = 0; i < node->child_count; i++) {
            printf("children[%d]=%s ", i, node->children[i]->filename);
        }
    }
    printf("\n");
}

// Find nodes from nodes
struct node *find_node(const char *path) {
    print_info("FindNode is called,tring to find %s\n", path);
    printf("-----------file_num--------------=%d\n", file_num);
    for (int i = 0; i < file_num; i++) {
        printf("i=%d,nodes[%d]->path=%s\n", i, i, nodes[i]->path);
        if (!strcmp(nodes[i]->path, path)) {
            printf("Lookup successed, %s was found.\n", nodes[i]->path);
            // PrintNode(nodes[i]);
            return nodes[i];
        }
    }
    printf("Lookup failed, %s does not exist in the file system.\n", path);
    return NULL;
}

// Find the names of all child nodes of parent
void FindChild(struct node *parent, char **child_names) {
    print_info("FindChild is called\n");
    if (parent->child_count == 0) {
        printf("path %s has no child\n", parent->path);
        return;
    }

    for (int i = 0; i < parent->child_count; i++) {
        child_names[i] = (char *)malloc(sizeof(parent->children[i]->filename));
        strcpy(child_names[i], parent->children[i]->filename);
    }
}

// Persist to local, not completed
void SerializeNode(struct node *node, FILE *fp) {
    print_info("SerializeNode is called");

    int filename_len = strlen(node->filename);
    fwrite(&filename_len, sizeof(int), 1, fp);
    fwrite(node->filename, sizeof(char), strlen(node->filename) + 1, fp);
    printf("ccc\n");

    if (node->contents == NULL) {
        int contents_len = 0;
        fwrite(&contents_len, sizeof(int), 1, fp);
    } else {
        int contents_len = strlen(node->contents);
        printf("aaa\n");
        fwrite(&contents_len, sizeof(int), 1, fp);
        printf("bbb\n");
        fwrite(node->contents, sizeof(char), strlen(node->contents) + 1, fp);
    }

    int path_len = strlen(node->path);
    fwrite(&path_len, sizeof(int), 1, fp);
    fwrite(node->path, sizeof(char), strlen(node->path) + 1, fp);
    /*fwrite(&node->mode, sizeof(mode_t), 1, fp);
    fwrite(&node->uid, sizeof(uid_t), 1, fp);
    fwrite(&node->gid, sizeof(gid_t), 1, fp);
    fwrite(&node->child_count, sizeof(int), 1, fp);
    fwrite(&node->atime, sizeof(time_t), 1, fp);
    fwrite(&node->mtime, sizeof(time_t), 1, fp);
    fwrite(&node->ctime, sizeof(time_t), 1, fp);*/

    for (int i = 0; i < node->child_count; i++) {
        SerializeNode(node->children[i], fp);
    }
}

// Persist to local, not completed
void SerializeTree(struct node *root) {
    print_info("SerializeTree is called");
    FILE *fp;
    char *filename = "serialized_file.txt";
    fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Error opening file!\n");
        return;
    }
    SerializeNode(root, fp);
    fclose(fp);
}

// Deserialize from local, incomplete
struct node *DeserializeNode(FILE *fp) {
    print_info("DeserializeNode is called");
    struct node *node = (struct node *)malloc(sizeof(struct node));
    int filename_len, contents_len, path_len;

    fread(&filename_len, sizeof(int), 1, fp);
    node->filename = (char *)malloc(filename_len + 1);
    fread(node->filename, sizeof(char), filename_len, fp);
    node->filename[filename_len] = '\0';

    fread(&contents_len, sizeof(int), 1, fp);
    if (contents_len == 0) {
        node->contents = NULL;
    } else {
        node->contents = (char *)malloc(contents_len + 1);
        fread(node->contents, sizeof(char), contents_len, fp);
        node->contents[contents_len] = '\0';
    }

    fread(&path_len, sizeof(int), 1, fp);
    node->path = (char *)malloc(path_len + 1);
    fread(node->path, sizeof(char), path_len, fp);  //
    node->path[path_len] = '\0';

    /*fread(&node->mode, sizeof(mode_t), 1, fp);
    fread(&node->uid, sizeof(uid_t), 1, fp);
    fread(&node->gid, sizeof(gid_t), 1, fp);

    fread(&node->atime, sizeof(time_t), 1, fp);
    fread(&node->mtime, sizeof(time_t), 1, fp);
    fread(&node->ctime, sizeof(time_t), 1, fp);*/

    /*fread(&node->child_count, sizeof(int), 1, fp);
    node->children = (struct node**)malloc(node->child_count * sizeof(struct
    node*)); for (int i = 0; i < node->child_count; i++) { node->children[i] =
    DeserializeNode(fp); node->children[i]->parent = node;
    }*/

    return node;
}

// Initialize the filesystem
void *kfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    print_info("init\n");
    cfg->kernel_cache = 1; // 启用内核缓存
    nodes[0] = create_node("/", S_IFDIR | 0775, NULL);
    return NULL;
}

// temporarily useless
int kfs_mknod(const char *path, mode_t mode, dev_t rdev) {
    print_info("kfs_mknod path = %s\n", path);
    // struct options nod;
    // nod.filename=strdup("new_file");
    // options_list[0]=nod;
    return 0;
}

// Create a new file in memory
int kfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    print_info("kfs_create path = %s\n", path);
    // find parent
    const char *parent_path = find_parent_dir(path);
    struct node *parent = find_node(parent_path);

    // create node
    struct node *new_node = create_node(path, mode, parent);

    printf("kfs_create: parent is %s.Print parent before add children.\n", parent->path);
    print_node(parent);
    // Add child nodes to parent
    parent->children[parent->child_count] = new_node;
    parent->child_count++;
    printf("kfs_create: parent is %s.Print parent after add children.\n", parent->path);
    print_node(parent);
    return 0;
}

// Create a new dir in memory
static int kfs_mkdir(const char *path, mode_t mode) {
    mode = S_IFDIR | 0775;
    print_info("kfs_mkdir path = %s\n", path);
    if ((mode & S_IFMT) == S_IFDIR)
        printf("S_ISDIR\n");
    else
        printf("mode=%d", mode);

    // find parent
    const char *parent_path = find_parent_dir(path);
    struct node *parent = find_node(parent_path);

    // create node
    struct node *new_node = create_node(path, mode, parent);

    // Add child nodes to parent
    parent->children[parent->child_count] = new_node;
    parent->child_count++;

    print_node(parent);
    printf("check node\n");
    print_node(new_node);
    return 0;
}

// open file
int kfs_open(const char *path, struct fuse_file_info *fi) {
    print_info("kfs_open path = %s\n", path);
    struct node *target_node = find_node(path);
    if (target_node == NULL) {
        printf("kfs_open:%s does not exist.\n", path);
        return -ENOENT;
    }
    fi->fh = (uint64_t)target_node;
    printf("kfs_open:%s succeed.\n", path);
    return 0;
}

// write
int kfs_write(const char *path, const char *buf, size_t len, off_t offset, struct fuse_file_info *fi) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    // fprintf(stderr, "kfs_write path = %s\n", path);
    struct node *target_node = (struct node *)fi->fh;
    int new_size = offset + len;  // Byte
    if (target_node->contents) {
        if (new_size > strlen(target_node->contents)) {  // only English
            target_node->contents = realloc(target_node->contents, new_size);
        }
    } else {
        target_node->contents = malloc(new_size);
    }
    // printf("offset=%ld\n",offset);
    memcpy(target_node->contents + offset, buf, len);
    // PrintNode(target_node);
    clock_gettime(CLOCK_MONOTONIC, &end);

    // calculating time
    printf("time elapsed: %ld ns\n", (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec));
    return len;
}

int kfs_truncate(const char *path, off_t length) {
    print_info("kfs_truncate path = %s\n", path);
    struct node *target_node = find_node(path);
    target_node->contents = realloc(target_node->contents, length);
    target_node->contents[length] = '\0';
    // PrintNode(target_node);
    return 0;
}

// Get file properties
static int kfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    // fprintf(stderr, "kfs_getattr path = %s\n", path);

    struct node *target_node = find_node(path);
    // PrintNode(target_node);
    if (find_node(path) == NULL) {
        printf("kfs_getattr failed,path=%s\n", path);
        return -ENOENT;
    } else {
        printf("kfs_getattr succeed,path=%s\n", path);

        if (target_node->contents != NULL)
            stbuf->st_size = strlen(target_node->contents);
        else
            stbuf->st_size = 0;
        stbuf->st_mode = target_node->mode;

        stbuf->st_atime = target_node->atime;
        stbuf->st_mtime = target_node->mtime;
        stbuf->st_ctime = target_node->ctime;
        return 0;
    }
}

// read file content
static int kfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    // fprintf(stderr, "kfs_read path = %s\n", path);
    struct node *target_node = (struct node *)fi->fh;

    if (target_node->contents == NULL) {
        return 0;
    }
    size_t contents_size = strlen(target_node->contents);
    if (offset < contents_size) {
        if (offset + size > contents_size) {
            size = contents_size - offset;
        }
        const char *contents = strdup(target_node->contents + offset);
        print_info("read contents: %s\n", contents);
        memcpy(buf, target_node->contents + offset, size);
    } else {
        size = 0;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("read time elapsed: %ld ns\n", (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec));
    return size;
}

// read files in directory
static int kfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags) {
    print_info("kfs_readdir path = %s\n", path);
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    // Find child nodes under the path
    struct node *parent = find_node(path);
    // char** child_names[MAX_CHILDREN];
    char **child_names = (char **)malloc(MAX_CHILDREN * sizeof(char *));
    printf("before findchild\n");
    print_node(parent);
    FindChild(parent, child_names);
    for (int i = 0; i < parent->child_count; i++) {
        printf("child_names[%d]=%s\n", i, child_names[i]);
        filler(buf, child_names[i], NULL, 0, 0);
    }
    for (int i = 0; i < parent->child_count; i++) {
        free(child_names[i]);
    }
    free(child_names);
    return 0;
}

// temporarily useless
int kfs_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) {
    print_info("kfs_utimens path = %s\n", path);
    return 0;
}

// temporarily useless
// int kfs_setattr(const char *path, const char ) {
//     fprintf(stderr, "kfs_setattr path = %s\n", path);
//     int res = 0;

//     return res;
// }

// temporarily useless
int kfs_release(const char *path, struct fuse_file_info *fi) {
    print_info("kfs_release path = %s\n", path);
    return 0;
}

// delete file
int kfs_unlink(const char *path) {
    // Find the node to be deleted
    struct node *node_to_delete = find_node(path);
    if (node_to_delete == NULL)
        return -ENOENT;
    // Remove the node from parent's children list
    struct node *parent = node_to_delete->parent;
    int i = 0;
    for (i = 0; i < parent->child_count; i++) {
        if (strcmp(parent->children[i]->path, path) == 0) {
            break;
        }
    }
    for (int j = i; j < parent->child_count - 1; j++) {
        parent->children[j] = parent->children[j + 1];  // subtree left shift
    }
    parent->child_count--;
    // Free the memory allocated for the node
    free(node_to_delete->path);
    free(node_to_delete->filename);
    if (node_to_delete->contents != NULL)
        free(node_to_delete->contents);

    // Remove the node from nodes
    int j;
    for (i = 0; i < file_num; i++) {
        if (nodes[i] == node_to_delete) {
            free(nodes[i]);
            for (j = i; j < file_num - 1; j++) {
                nodes[j] = nodes[j + 1];
            }
            file_num--;
            break;
        }
    }

    return 0;
}

// delete dir
int kfs_rmdir(const char *path) {
    // Find the node to be deleted
    struct node *node_to_delete = find_node(path);
    if (node_to_delete == NULL)
        return -ENOENT;
    // Remove the node from parent's children list
    struct node *parent = node_to_delete->parent;
    int i = 0;
    for (i = 0; i < parent->child_count; i++) {
        if (strcmp(parent->children[i]->path, path) == 0) {
            break;
        }
    }
    for (int j = i; j < parent->child_count - 1; j++) {
        parent->children[j] = parent->children[j + 1];  // subtree left shift
    }
    parent->child_count--;
    // remove children node
    char **child_names = (char **)malloc(MAX_CHILDREN * sizeof(char *));
    FindChild(node_to_delete, child_names);
    for (int k = 0; k < node_to_delete->child_count; k++) {
        struct node *child = find_node(child_names[k]);
        if ((child->mode & S_IFMT) == S_IFDIR) {
            kfs_rmdir(child->path);
        } else {
            kfs_unlink(child->path);
        }
    }

    // Free the memory allocated for the node
    free(node_to_delete->path);
    free(node_to_delete->filename);
    if (node_to_delete->contents != NULL)
        free(node_to_delete->contents);

    // Remove the node from nodes
    int j;
    for (i = 0; i < file_num; i++) {
        if (nodes[i] == node_to_delete) {
            free(nodes[i]);
            for (j = i; j < file_num - 1; j++) {
                nodes[j] = nodes[j + 1];
            }
            file_num--;
            break;
        }
    }

    return 0;
}

// Execute when destroyed, call the persistence operation at this time
void kfs_destroy(void *private_data) {
    //
    print_info("kfs_destroy\n");
    //
    /*FILE* fp;
    char* filename = "/home/matrix/project/libfuse/MyFuseSys/serialized_file.txt";
    fp = fopen(filename, "w");
    if (fp == NULL) {
            printf("Error opening file!\n");
            return 1;
    }*/

    SerializeTree(find_node("/"));
    print_info(
        "Serialization has been completed, and the serialized file is saved "
        "in serialized_file.txt in the root directory of the project.\n");
}

// rename file or dir
int kfs_rename(const char *old_path, const char *new_path, unsigned int flags) {
    int ret = 0;

    // Perform file system specific operations to rename a file.
    struct node *target = find_node(old_path);
    free(target->filename);
    free(target->path);
    target->filename = get_file(new_path);
    target->path = strdup(new_path);
    return ret;
}

static const struct fuse_operations kfs_op = {
    .init = kfs_init,        // 初始化文件系统
    .mknod = kfs_mknod,      // 创建文件或目录节点
    .create = kfs_create,    // 创建文件
    .mkdir = kfs_mkdir,      // 创建目录
    .open = kfs_open,        // 打开文件
    .write = kfs_write,      // 写入文件
    .release = kfs_release,  // 文件释放或关闭
    .getattr = kfs_getattr,  // 获取文件属性
    .utimens = kfs_utimens,  // 修改文件的时间戳
    .readdir = kfs_readdir,  // 读取目录内容
    .read = kfs_read,        // 读取文件
    .unlink = kfs_unlink,    // 删除文件
    .rmdir = kfs_rmdir,      // 删除目录
    .destroy = kfs_destroy,  // 文件系统销毁
    .rename = kfs_rename,    // 重命名文件或目录
};

int main(int argc, char *argv[]) {
    int ret;
    // char user_input[256];
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    ret = fuse_main(args.argc, args.argv, &kfs_op,
                    NULL);  // Entrance to the FUSE library
    fuse_opt_free_args(&args);
    return ret;
}
