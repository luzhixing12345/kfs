# kfs

![badge.svg](https://github.com/luzhixing12345/kfs/actions/workflows/main.yaml/badge.svg)

kfs 是一个基于 FUSE 的类 ext4 文件系统, 同时可以利用 Virtio(vhost-user) 协议实现虚拟机(VM)和宿主机之间高效共享文件系统.

![20240731230954](https://raw.githubusercontent.com/learner-lu/picbed/master/20240731230954.png)

本项目共包含四个部分

- `kfs`: 文件系统, 可以挂载磁盘镜像并映射文件系统
- `mkfs`: 磁盘格式化工具, 可以将磁盘/文件格式化为 kfs 文件系统需要的格式
- `kfsd`: 通过 Virtio(vhost-user) 协议实现虚拟机(VM)和宿主机之间通信, 将 VM 请求转发至 host 文件系统处理
- `kfsctl`: 命令行交互工具, 可以查看挂载的文件系统状态, 快照, 恢复, 磁盘碎片整理等操作

## 编译运行

安装依赖

```bash
sudo apt-get install fuse3 libfuse3-dev pkg-config libcap-ng-dev libseccomp-dev
```

编译得到文件系统 `src/kfs` 和磁盘格式化程序 `mkfs/mkfs` 和命令行交互工具 `kfsctl/kfsctl`

```bash
make
```

```bash
cd kfsd
cargo build --release
```

创建 `disk.img` 文件(1000 MiB), 并使用 `mkfs/mkfs` 将其格式化为 ext4 文件系统格式

```bash
make disk
```

挂载文件系统到 `tmp/` 下

```bash
mkdir tmp
make run
```

可以进入 `tmp/` 目录下进行操作, 例如创建/打开/读取/修改文件等等, 所有修改都会保存在 `disk.img` 中, 所有的操作日志保存在 `kfs.log` 中

结束操作后使用取消挂载文件系统即可

```bash
umount tmp
```

或者以调试模式挂载并执行

```bash
make debug_run
```

测试

```bash
make test
```

## VM 启动

Guest kernel 编译时需要添加 virtiofs 支持

```txt
CONFIG_VIRTIO_FS
CONFIG_FUSE
```

启动 virtfsd

```bash
cargo run -- --shared-dir <dir> --socket-path /tmp/vhostqemu
```

qemu 启动时添加 `chardev` 用于创建一个 socket(/tmp/vhostqemu) 实现通信. 创建一个 `vhost-user-fs-pci` 的 device, 添加 tag 为 myfs 用于后续挂载

```bash
qemu-system-x86_64 \
        -kernel <your-kernel> \
        -drive <your-drive>
        -chardev socket,id=char0,path=/tmp/vhostqemu \
        -device vhost-user-fs-pci,queue-size=1024,chardev=char0,tag=myfs \
        -m 4G -object memory-backend-file,id=mem,size=4G,mem-path=/dev/shm,share=on \
        -numa node,memdev=mem
```

```bash
sudo mount -t virtiofs myfs <mountpoint>
```

<!-- ## TODO

- [ ] 完善测试
- [ ] ext4 日志
- [ ] 支持并发
- [ ] 大文件读写
- [ ] 目录 Hash
- [ ] 高效的磁盘bitmap选择算法
- [ ] 快照
- [ ] 校验和
- [ ] 重复数据删除
- [ ] 压缩
- [ ] 延迟分配 -->

## 文档

项目技术文档见 `md-docs/`, 在线阅读: [kfs document](https://luzhixing12345.github.io/kfs/)

关于细节见 impl 部分

## 实现概述

在远程文件系统中,**transport(传输)**和**protocol(协议)**是两个核心概念,它们共同决定了数据如何在客户端和服务器之间传输,以及如何进行通信和交互.

- **Transport** 指数据在网络上传输的方式和路径.在远程文件系统中,传输层负责确保数据包能够在客户端和服务器之间可靠地传递.选择合适的传输协议取决于系统的需求,例如: TCP/IP, USB, RDMA, 通过消息传递或者共享内存等方式
- **Protocol** 指在客户端和服务器之间进行通信的规则和格式.它定义了消息的结构、命令的含义、错误处理机制等. 例如 NFS, CIFS, SSH 等

![20240731211711](https://raw.githubusercontent.com/learner-lu/picbed/master/20240731211711.png)

那么虚拟化场景下有什么特别的么? 注意到所有虚拟机**共用同一个物理机**, 他们的**物理内存实际上是共享的**, 由 VMM 统一管理和分配, 也就是不需要高开销的网络通信而可以通过一种高效的**内存映射**的方式完成通信

## vhost-user

最知名的虚拟化平台 QEMU 采用 vhost-user 协议. vhost-user 是一种用于用户态进程与内核态虚拟化组件(如KVM/QEMU)之间进行高速数据传输的通信协议.它最常见的用途是在虚拟机和虚拟设备(如虚拟网络接口、虚拟磁盘)之间实现高效的I/O操作

通常由 qemu 的 virtio-net 的实现虚拟网络通信设备作为 vhost-user 协议的 frontend 部分, 本项目实现了 vhost-user 协议的后端(backend) 部分, 与 VM 通信握手

![20240731232130](https://raw.githubusercontent.com/learner-lu/picbed/master/20240731232130.png)

## kfsd

传统 FUSE 的访问流程如下

- FUSE_INIT to creat session
- FUSE_LOOPUP(FUSE_ROOT_ID, "foo") -> nodeid
- FUSE_OPEN(nodeid, O_RDONLY) -> fh
- FUSE_READ(fh, offset, &buf, sizeof(buf)) -> nbytes

能否避免/减少与 kfsd 的通信开销? 能否避免与主机频繁的拷贝数据 from/to, `DAX` (Direct Access) 是virtiofs中的一项重要特性. 可以将数据可以直接从存储设备映射到用户空间文件区域

- 将文件区域映射到 guest 的内存区域
- 允许 guest mmap 访问内存区域

启用 DAX 之后可以充分利用主机的 page cache, pte 等加速访存操作, 避免通信和数据拷贝. 

- FUSE_INIT to creat session
- FUSE_LOOPUP(FUSE_ROOT_ID, "foo") -> nodeid
- FUSE_OPEN(nodeid, O_RDONLY) -> fh
- FUSE_SETUPMAPPING(fh, offset, len, addr)
- Memory access to [addr, addr+len]

## 参考

- [Linux 文件系统(一):抽象](https://www.bilibili.com/video/BV1jM411W7jV)
- [动手实现一个文件系统,提高自己代码能力,加深对底层的理解,探索自己的就业新可能](https://www.bilibili.com/video/BV1eV411A7gw)
  - [好文分享:EXT文件系统机制原理详解](https://www.51cto.com/article/603104.html)
  - [Project 06: Simple File System](https://www3.nd.edu/~pbui/teaching/cse.30341.fa19/project06.html)
- [NTFS 是目前最先进的文件系统吗?](https://www.zhihu.com/question/20619659)
- [libfuse](https://github.com/libfuse/libfuse)
- [simplefs](https://github.com/sysprog21/simplefs)
- [扒一扒 Linux 文件系统的黑历史](https://zhuanlan.zhihu.com/p/28828826)
- [fuse-backend-rs](https://github.com/cloud-hypervisor/fuse-backend-rs)
- [300行代码带你实现一个Linux文件系统](https://zhuanlan.zhihu.com/p/579011810)
- [动手写一个简单的文件系统](https://www.jianshu.com/p/8966d121263b)
- [Assignment 11: File system in libfuse](https://course.ccs.neu.edu/cs3650sp22/a11.html)
- [文件系统](https://realwujing.github.io/linux/kernel/%E6%96%87%E4%BB%B6%E7%B3%BB%E7%BB%9F/)
- [星星说编程](https://space.bilibili.com/50657960/channel/series)
