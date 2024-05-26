# 文件系统的权限管理

## 1.背景介绍

* UNIX/Linux系统采用了经典的用户-组-其他(User-Group-Others, UGO)权限模型,通过读(read, `r`)、写(write, `w`)、执行(execute, `x`)三种基本权限组合,赋予文件或目录不同的访问权限.
* 每个文件或目录都关联一个所有者(owner)、所属组(group),以及其他用户(others),每类用户对应一套权限设置.

* 在文件系统中,权限管理是一个关键特性,它控制着不同用户对文件和目录的访问.
* 本文档将介绍如何在基于FUSE的用户空间文件系统中实现权限管理.

## 2.文件和目录权限

在Linux系统中,每个文件和目录都有一组权限,决定了谁可以读、写或执行它们.这些权限通常表示为一个字符串,如`-rwxr-xr--`,其中:

- 第一个字符表示文件类型(如`-`代表普通文件,`d`代表目录).
- 接下来的三个字符**rwx**分别代表文件所有者的读(r)、写(w)和执行(x)权限.
- 紧随其后的三个字符**r-x**代表文件所属组的权限.
- 最后三个字符**r--**代表其他所有用户的权限.

> 当新创建文件和目录时,就由**umask**来设置新创建的文件和目录的默认权限.

## 3.切换用户

有时,需要具有另一个用户的身份.比如我们想要得到超级用户特权,来执行一些管理任务,有时比如说测试一个帐号,也需要变成另一个普通用户. 有三种方式,可以拥有多重身份:

- **注销系统并以其他用户身份重新登录系统**
  - 这个注销登录其他用户和 Windows 是差不多的,所以这里就不介绍了.
- **使用 su 命令**
  - 如,使用 `su + 用户名` 命令切换到其他用户.需输入要切换成用户的用户密码,切换过去完成了相应的任务后,可以使用 `exit` 命令退出,重新回到之前的用户.
- **使用 sudo 命令**
  - sudo 命令主要是使用 **root 用户**来执行命令,命令格式是:`sudo + 命令`.使用 sudo 时需输入的密码不是 root 用户的密码,而是当前用户的密码.

## 4.实现权限管理

### 4.1代码解析

下面的C函数`inode_check_permission`根据用户的身份和请求的操作模式,检查并决定是否允许对指定inode(索引节点,代表文件或目录的元数据结构)的访问.

**函数参数说明:**

- `inode`: 指向EXT4文件系统中inode结构的指针,包含文件的所有权信息(如所有者UID、所属组GID及权限位).
- `mode`: 用户尝试进行的操作模式,定义为`RD_ONLY`(只读)、`WR_ONLY`(只写)、`RDWR`(读写)中的一个.

```c
// TODO check user permission
int inode_check_permission(struct ext4_inode *inode, access_mode_t mode) {
    // UNIX permission check
    struct fuse_context *cntx = fuse_get_context(); //获取当前用户的上下文
    uid_t uid = cntx->uid;
    gid_t gid = cntx->gid;
    DEBUG("check inode & user permission on op mode %d", mode);
    DEBUG("inode uid %d, inode gid %d", inode->i_uid, inode->i_gid);
    DEBUG("user uid %d, user gid %d", uid, gid);

    // 检查用户是否是超级用户
    if (uid == 0) {
        INFO("Permission granted");
        return 0; // root用户允许所有操作
    }

    // 根据访问模式、文件所有者检查权限(只读、只写、读写)
    if (inode->i_uid == uid) {
        if ((mode == RD_ONLY && (inode->i_mode & S_IRUSR)) || (mode == WR_ONLY && (inode->i_mode & S_IWUSR)) ||
            (mode == RDWR && ((inode->i_mode & S_IRUSR) && (inode->i_mode & S_IWUSR)))) {
            INFO("Permission granted");
            return 0;  // 允许操作
        }
    }


    // 组用户检查
    if (inode->i_gid == gid) {
        if ((mode == RD_ONLY && (inode->i_mode & S_IRGRP)) ||
            (mode == WR_ONLY && (inode->i_mode & S_IWGRP)) ||
            (mode == RDWR && ((inode->i_mode & S_IRGRP) && (inode->i_mode & S_IWGRP)))) {
            INFO("Permission granted");
            return 0; // 允许操作
        }
    }
    
    // 检查其他用户权限
    if ((mode == RD_ONLY && (inode->i_mode & S_IROTH)) ||
        (mode == WR_ONLY && (inode->i_mode & S_IWOTH)) ||
        (mode == RDWR && ((inode->i_mode & S_IROTH) && (inode->i_mode & S_IWOTH)))) {
        INFO("Permission granted");
        return 0;  // 允许操作
    }

    return -EACCES; // 访问拒绝
}
```

### 4.2功能说明

#### (1)获取用户上下文

使用`fuse_get_context()`获取当前发起操作的用户上下文,包括用户ID(`uid`)和组ID(`gid`).

* **id**:是一个命令行工具,用于显示当前用户的身份号码,包括用户ID(UID)和组ID(GID).在FUSE文件系统中,这些身份号码通常用于权限检查.

* **uid**:用户身份号码

* **gid**:某用户所属的用户组号码,该用户加入用户组后就拥有了这些用户组成员共同拥有的权限.

```c
struct fuse_context *ctx = fuse_get_context();
uid_t uid = ctx->uid;  //用户ID
gid_t gid = ctx->gid;  //组ID
```

#### (2)inode权限检查

* 在执行任何文件操作之前,需要先检查当前用户的身份是否有权限执行该操作,通常在每个文件操作的回调函数中完成.

* 在FUSE中,inode通常包含有关文件权限的信息.我们需要实现一个函数来检查给定用户是否有权限访问inode.

##### ①超级用户检查

命令行工具su和sudo常用来切换超级用户.

- **su**(替代用户)允许用户以另一个用户的身份运行shell.通常用于切换到root用户或其他具有更高权限的用户.

- **sudo**(超级用户执行)允许授权的用户以另一个用户(通常是root)的身份执行命令,而无需共享root密码.

如果当前用户是root(`uid == 0`),由于root具有系统最高权限,直接允许所有操作.

##### ②所有者权限检查

比较inode的所有者UID与当前用户的UID.如果匹配且请求的操作模式与inode权限位(如`S_IRUSR`读权限、`S_IWUSR`写权限)相符合,则允许访问.

##### ③组用户权限检查

如果inode的所属组GID与当前用户所在的组GID相同,且权限位(如`S_IRGRP`、`S_IWGRP`)满足请求的操作模式,则权限被授予.

##### ④其他用户权限检查

除了uid和gid外,还要检查其他用户

最后,检查inode针对"其他人"的权限位(如`S_IROTH`、`S_IWOTH`).如果这些权限位允许请求的操作,则访问被许可.

##### ⑤权限拒绝处理

如果以上所有条件都不满足,则通过返回`-EACCES`错误码,表明访问被拒绝.

#### (3)更改权限和所有权

我们还可以实现`op_chmod`和`op_chown`等操作来允许用户更改文件权限和所有权.

##### op_chmod更改文件权限

更改文件的权限有两种表示方法,一种是用数字表示权限,另一种是用 `rwx` 表示权限.为了说明怎么用数字表示权限,我们得先来了解一下八进制.`chmod` 代表"更改模式"(change mode),是一个用于更改文件或目录权限的命令行工具.它允许用户设置读(r)、写(w)和执行(x)权限.

##### op_chown更改文件所有者

**chown**是一个命令行工具,用于更改文件或目录的所有者.这通常需要管理员权限.

**chgrp**:是一个更改组的命令行工具,用于更改文件或目录的组所有权.与`chown`类似,这通常需要管理员权限.

## 5.完善权限检查逻辑

为了使权限检查功能完备,我们需要在`inode_check_permission`函数中加入实际的权限验证代码.这通常涉及以下步骤:

1. **权限掩码比对**:利用inode中的权限位(如读权限为4,写权限为2,执行权限为1)与用户身份进行比对.对于文件所有者、同组用户、其他用户分别有不同的权限设定.
2. **特殊权限处理**:考虑SUID、SGID、sticky bit等特殊权限标志的影响.
3. **错误处理**:当权限检查不通过时,应当有明确的错误处理逻辑,例如返回非零值表示权限拒绝.

## 6.结论

通过实现上述步骤,我们可以在基于FUSE的用户空间文件系统中实现基本的权限管理.这确保了只有合适的用户才能访问或修改文件系统中的资源.

## 7.参考

https://zhuanlan.zhihu.com/p/61196860