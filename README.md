# kfs

设计实现一个Linux内核模块,此模块完成如下功能

- [ ] 将新创建的文件系统的操作接口和VFS对接.
- [ ] 实现新的文件系统的超级块、dentry、inode的读写操作.
- [ ] 实现新的文件系统的权限属性,不同的用户不同的操作属性.
- [ ] 实现和用户态程序的对接,用户程序
- [ ] 设计实现一个用户态应用程序,可以将一个块设备(可以用文件模拟)格式化成自己设计的文件系统的格式.
- [ ] 设计一个用户态的测试用例应用程序,测试验证自己的文件系统的open/read/write/ls/cd 等通常文件系统的访问.

## 编译运行

```bash
make
```

## 文档

## 参考

- [Linux 文件系统(一):抽象](https://www.bilibili.com/video/BV1jM411W7jV)
- [动手实现一个文件系统,提高自己代码能力,加深对底层的理解,探索自己的就业新可能](https://www.bilibili.com/video/BV1eV411A7gw)
- [NTFS 是目前最先进的文件系统吗?](https://www.zhihu.com/question/20619659)
- [libfuse](https://github.com/libfuse/libfuse)
- [simplefs](https://github.com/sysprog21/simplefs)