
# kfsd

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

![20240731230954](https://raw.githubusercontent.com/learner-lu/picbed/master/20240731230954.png)

## 参考

- [[2019] virtio-fs: A Shared File System for Virtual Machines by Stefan Hajnoczi](https://www.youtube.com/watch?v=969sXbNX01U)
- [virtio-fs A shared file system for virtual machines](https://www.youtube.com/watch?v=EIVOzTsGMMI)
- [gitlab virtio-fs](https://virtio-fs.gitlab.io/howto-qemu.html)