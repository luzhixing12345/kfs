
# FUSE介绍

## 什么是FUSE?

FUSE(Filesystem in Userspace)是一个允许非特权用户在用户空间创建自己的文件系统的工具.

* 它提供了一种机制,使得开发者可以在用户空间而不是内核空间实现文件系统.

- 开发者可以利用现有的用户空间编程语言和工具来创建文件系统,而不需要深入了解Linux内核的复杂性.

* 编写FUSE文件系统时,只需要内核加载了fuse内核模块即可,不需要重新编译内核.适合多种应用场景,如读写虚拟文件系统等.

## FUSE的工作原理

FUSE通过一个轻量级的内核模块作为桥梁,实现了用户空间程序与操作系统VFS(Virtual File System)层的沟通.它通过一个名为`fuse`的内核模块与操作系统的内核进行通信.当应用程序尝试访问文件系统时,内核会将请求转发给FUSE内核模块,该模块再将请求传递给用户空间中的文件系统实现.

### 主要组件

- **FUSE内核模块**:这是与操作系统内核交互的组件,负责将从VFS发的文件系统请求传递给用户空间进程;及将接收到的用户空间进程处理完的请求返回给VFS.

- **用户空间文件系统**:这是实际实现文件系统逻辑的组件,可以是任何用户空间程序.

- **FUSE库libfuse**:这是提供给用户空间文件系统的库,它提供了一组API来简化文件系统实现.负责和内核空间通信,接收来自/dev/fuse的请求,并将其转化为一系列的函数调用,将结果写回/dev/fuse;

  提供的函数可以对fuse文件系统进行挂载卸载、从linux内核读取请求以及发送响应到内核.

![img](https://imgconvert.csdnimg.cn/aHR0cHM6Ly9tbWJpei5xcGljLmNuL21tYml6X3BuZy9kNGhvWUpseE9qTnNvaWNRQkUwM01aRDBrWjNmY3VpYWVRZzJmV1RlNFlWV3RUYko5aWN1cG1iZ1IwZGd1RUlrTTloTzZzaWJQdU80VTlFNzlpYWczWWljdlE4US82NDA?x-oss-process=image/format,png)

上图展示了fuse的构架.当application挂在fuse文件系统上,并且执行一些系统调用时,VFS会将这些操作路由至fuse driver,fuse driver创建了一个fuse request结构体,并把request保存在请求队列中.此时,执行操作的进程会被阻塞,同时fuse daemon通过读取/dev/fuse将request从内核队列中取出,并且提交操作到底层文件系统中(例如 EXT4 或 F2FS).当处理完请求后,fuse daemon会将reply写回/dev/fuse,fuse driver此时把requset标记为completed,最终唤醒用户进程.

### 工作流程

1. **挂载请求与初始化**:
   - 用户或系统管理员通过命令行工具(如`mount.fuse`或`fusermount`)发出指令,请求挂载一个FUSE文件系统到特定的目录(挂载点).
   - 这个挂载操作触发FUSE内核模块的加载(如果尚未加载),该模块是FUSE实现的核心组件之一,负责在内核空间与用户空间之间建立通信机制.
2. **FUSE内核模块初始化**:
   - FUSE内核模块注册自身为VFS(Virtual File System)中的一个文件系统类型,并创建一个特殊的字符设备文件(如`/dev/fuse`),用于后续的用户空间通信.
   - 内核模块配置好与用户空间守护进程的通信参数,包括挂载选项、权限设置等.
3. **用户空间守护进程启动**:
   - FUSE用户空间守护进程(即开发者实现的文件系统逻辑程序)开始运行,它将通过打开并读写`/dev/fuse`设备文件与内核进行交互.
   - 守护进程通过FUSE库(libfuse)注册一系列回调函数,这些函数对应文件系统操作,如打开文件(`open`)、读取(`read`)、写入(`write`)、列出目录内容(`readdir`)等.
4. **系统调用与请求转发**:
   - 当应用程序执行如`open`, `read`, `write`等系统调用来访问挂载点下的文件时,这些调用首先到达内核的VFS层.
   - VFS识别出这是针对FUSE挂载点的请求,于是将调用转发给FUSE内核模块,而不是直接处理.
5. **消息封装与传递**:
   - FUSE内核模块将这些系统调用转换成FUSE协议的格式,封装成消息,然后通过`/dev/fuse`设备文件发送到用户空间.
   - 这些消息包含了请求的操作类型、参数以及必要的上下文信息.
6. **用户空间处理逻辑**:
   - FUSE守护进程读取消息,并根据消息内容调用对应的回调函数.
   - 回调函数执行实际的文件系统操作逻辑,可能涉及读写真实数据存储、网络通信(如云存储)、加密解密等复杂处理.
7. **响应生成与返回**:
   - 处理完成后,守护进程构建响应消息,包含操作结果(成功或失败状态、返回数据等),并通过`/dev/fuse`回传给内核模块.
   - FUSE内核模块将用户空间的响应转换回内核可理解的格式,并通过VFS最终将结果返回给发起原始系统调用的应用程序.

## FUSE的优势

- **跨平台**:FUSE支持多种操作系统,使得文件系统实现可以跨平台使用.
- **灵活性**:开发者可以使用任何编程语言来实现文件系统,增加了灵活性.
- **易于开发**:发者可以在用户空间实现文件系统,fuse的库文件简单、安装也简单,不需要重新编译内核
- **安全**:相比直接在内核空间编程,用户空间编程更加简单和安全.

## FUSE的应用场景

- **网络文件系统**:实现访问远程文件的文件系统.
- **加密文件系统**:创建加密存储的文件系统.
- **压缩文件系统**:提供自动压缩和解压文件的文件系统.
- **虚拟化文件系统**:为虚拟机提供文件系统支持.
- **用户定义的文件系统**:根据特定需求定制文件系统.

### 应用案例

- **数据备份与同步**: 如rsync, Dropbox等利用FUSE进行无缝集成.
- **云存储接入**: 如S3FS,将云存储挂载为本地文件系统.
- **加密文件系统**: EncFS等项目,实现透明文件加密.
- **虚拟化与容器**: 在Docker容器中使用FUSE挂载特殊文件系统.
- **跨平台兼容**: 如WinFsp,使Windows支持FUSE文件系统.

## FUSE的局限性

- **性能**:由于文件系统是在用户空间实现的,可能会比内核空间实现的文件系统慢.
- **权限问题**:FUSE文件系统可能无法访问内核空间的某些资源.
- **稳定性**:用户空间的错误可能导致整个文件系统崩溃.

## FUSE的安装和使用

### 安装

在Linux上,可以通过包管理器安装FUSE.如在Ubuntu上,可以使用以下命令安装:

```
sudo apt-get install fuse
```

在macOS上,可以使用Homebrew安装:

```
brew install osxfuse
```

### 使用

1. **创建文件系统**:编写文件系统的代码,实现所需的功能.比如如何响应读、写、打开、关闭等操作.
2. **编译文件系统**:编译代码,生成可执行文件.
3. **挂载文件系统**:使用`fusermount`命令挂载文件系统.
4. **访问和使用挂载的文件系统**:一旦挂载成功,就可以像操作任何其他文件系统一样来使用这个挂载点.可以使用文件管理器浏览、编辑文件等.
5. **卸载FUSE文件系统**:使用完毕后,通过命令卸载文件系统:

- ```bash
  1fusermount -u /mount/point
  ```

## 结论

FUSE提供了一种强大且灵活的方式来创建自定义文件系统,它降低了文件系统开发的门槛,使得更多的开发者能够参与到文件系统的设计和实现中.尽管存在一些局限性,但FUSE在许多应用场景下仍然是一个有价值的工具.

## 参考

[http://fuse.sourceforge.net/](http://fuse.sourceforge.net/?spm=5176.28103460.0.0.423c3da2xGXQya)

[用户空间中的文件系统 - 维基百科,自由的百科全书 (wikipedia.org)](https://en.wikipedia.org/wiki/Filesystem_in_Userspace)

[EncFS Documentation](https://github.com/vgough/encfs/wiki?spm=5176.28103460.0.0.423c3da2xGXQya)

https://blog.csdn.net/m0_65931372/article/details/126253082

https://blog.csdn.net/daocaokafei/article/details/115414557

[1]Bharath Kumar Reddy Vangoor;Vasily Tarasov;Erez Zadok.To FUSE or not to FUSE: performance of user-space file systems[A].15th USENIX Conference on File and Storage Technologies (FAST 2017)[C],2017

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
- [sshfs](https://github.com/libfuse/sshfs)