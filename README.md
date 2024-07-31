# kfs

![badge.svg](https://github.com/luzhixing12345/kfs/actions/workflows/main.yaml/badge.svg)

kfs 是一个基于 FUSE 的类 ext4 文件系统, 同时可以利用 Virtio(vhost-user) 协议实现虚拟机(VM)和宿主机之间高效共享文件系统. 本项目共包含四个部分

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
make run && make test
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

项目文档见 [kfs 项目文档](./report.md), 视频介绍[链接](https://pan.baidu.com/s/1l5R2AKrJvmYyEZOhUYFlZA) 提取码:3rcb 

项目技术文档见 `md-docs/`, 在线阅读: [kfs document](https://luzhixing12345.github.io/kfs/)

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
