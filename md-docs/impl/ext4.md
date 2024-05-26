
# ext4

> ext 文件系统的配置文件位于 `/etc/mke2fs.conf`

## EXT4 磁盘格式

### 启动块(Boot Block)

大小就是1KB,由PC标准规定,用来存储磁盘分区信息和启动信息, 任何文件系统都不能使用该块

### 超级块(Super Block)

描述整个分区的文件系统信息,例如块大小文件系统版本号、上次mount的时间等等.超级块在每个块组的开头都有一份拷贝.

### 块组描述符表(GDT)

GDT(Group Descriptor Table) 由很多块组描述符组成,整个分区分成多少个块组就对应有多少个块组描述符.每个块组描述符存储一个块组的描述信息,包括inode表哪里开始,数据块从哪里开始,空闲的inode和数据块还有多少个等.块组描述符表在每个块组的开头也都有一份拷贝,这些信息是非常重要的,因此它们都有多份拷贝.

### 块位图 (Block Bitmap)

块位图就是用来描述整个块组中哪些块已用哪些块空闲的,本身占一个块,其中的每个bit代表本块组中的一个块,这个bit为1表示该块已用,这个bit为0表示该块空闲可用

### inode位图 (inode Bitmap)

和块位图类似,本身占一个块,其中每个bit表示一个inode是否空闲可用.

### inode表 (inode Table)

文件类型(常规、目录、符号链接等),权限,文件大小,创建/修改/访问时间等信息存在inode中,每个文件都有一个inode.

### 数据块(Data Block)

- **常规文件**: 文件的数据存储在数据块中.
- **目录**: 该目录下的所有文件名和目录名存储在数据块中.(注意:文件名保存在它所在目录的数据块中,其它信息都保存在该文件的inode中)
- **符号链接**: 如果目标路径名较短则直接保存在inode中以便更快地查找,否则分配一个数据块来保存
- **设备文件、FIFO和socket等特殊文件**: 没有数据块,设备文件的主设备号和次设备号保存在inode中

## EXT4

ext4 使用 extend 取代了 ext2 和 ext3 使用的传统块映射方案.盘区是一系列连续的物理块,可提高大文件性能并减少碎片.

其中 `ext4_extent` 的数据结构如下所示, 根据 [ext4 文档](https://kernel.org/doc/html/v6.6/filesystems/ext4/dynamic.html#extent-tree) 信息可知, 其中 `ee_len` 字段用于表示范围覆盖的块数. 如果此字段的值为 <= 32768,则认为该块没有被使用. 如果字段的值为 > 32768,则数据块正在使用,实际数据块长度为 `ee_len - 32768`.

因此通过计算可以得到, 扩展块的最大大小为 2^(16-1) x 2^12(4KB) = 128MB, 因此 ext4 中的单个扩展块最多可以映射 128 MiB 的连续空间

```c
struct ext4_extent {
    __le32  ee_block;       /* 文件逻辑块号的第一个块,表示这个扩展的起始块 */
    __le16  ee_len;         /* 这个扩展覆盖的数据块数量 */
    __le16  ee_start_hi;    /* 物理块号的高 16 位 */
    __le32  ee_start_lo;    /* 物理块号的低 32 位 */
};
```

```c
struct ext4_inode {
    __le16 i_mode;        /* 文件模式,包括文件类型和文件的访问权限 */
    __le16 i_uid;         /* 文件所有者的低16位用户ID */
    __le32 i_size_lo;     /* 文件大小的低32位 */
    __le32 i_atime;       /* 文件最后访问时间 */
    __le32 i_ctime;       /* inode最后改变时间 */
    __le32 i_mtime;       /* 文件最后修改时间 */
    __le32 i_dtime;       /* 文件删除时间 */
    __le16 i_gid;         /* 文件所有组的低16位组ID */
    __le16 i_links_count; /* 指向该inode的硬链接数量 */
    __le32 i_blocks_lo;   /* 文件占用的块数的低32位 */
    __le32 i_flags;       /* 文件标志,用于存储文件的特定属性 */
    union {
        struct {
            __le32 l_i_version; /* 用于NFS的文件版本号 */
        } linux1;
        struct {
            __u32 h_i_translator; /* Hurd的特定字段 */
        } hurd1;
        struct {
            __u32 m_i_reserved1; /* Masix的保留字段 */
        } masix1;
    } osd1;                        /* 操作系统依赖字段1 */
    __le32 i_block[EXT4_N_BLOCKS]; /* 指向文件数据块的指针数组 */
    __le32 i_generation;           /* 文件版本号,用于NFS */
    __le32 i_file_acl_lo;          /* 文件访问控制列表的低32位 */
    __le32 i_size_high;            /* 文件大小的高32位 */
    __le32 i_obso_faddr;           /* 已废弃的碎片地址 */
    union {
        struct {
            __le16 l_i_blocks_high;   /* 文件占用块数的高16位 */
            __le16 l_i_file_acl_high; /* 文件ACL的高16位 */
            __le16 l_i_uid_high;      /* 用户ID的高16位 */
            __le16 l_i_gid_high;      /* 组ID的高16位 */
            __u32 l_i_reserved2;      /* 保留字段 */
        } linux2;
        struct {
            __le16 h_i_reserved1; /* 已废弃的碎片编号/大小 */
            __u16 h_i_mode_high;  /* 文件模式的高16位 */
            __u16 h_i_uid_high;   /* 用户ID的高16位 */
            __u16 h_i_gid_high;   /* 组ID的高16位 */
            __u32 h_i_author;     /* Hurd的作者字段 */
        } hurd2;
        struct {
            __le16 m_i_reserved1;     /* 已废弃的碎片编号/大小 */
            __le16 m_i_file_acl_high; /* 文件ACL的高16位 */
            __u32 m_i_reserved2[2];   /* Masix的保留字段 */
        } masix2;
    } osd2;                /* 操作系统依赖字段2 */
    __le16 i_extra_isize;  /* inode结构体扩展大小 */
    __le16 i_pad1;         /* 对齐填充 */
    __le32 i_ctime_extra;  /* inode最后改变时间的额外信息(纳秒和纪元) */
    __le32 i_mtime_extra;  /* 文件最后修改时间的额外信息(纳秒和纪元) */
    __le32 i_atime_extra;  /* 文件最后访问时间的额外信息(纳秒和纪元) */
    __le32 i_crtime;       /* 文件创建时间 */
    __le32 i_crtime_extra; /* 文件创建时间的额外信息(纳秒和纪元) */
    __le32 i_version_hi;   /* 64位版本号的高32位 */
};
```

```c
struct ext4_extent_header {
    __le16 eh_magic;      /* 魔数,用于识别扩展索引的格式 */
    __le16 eh_entries;    /* 有效的条目数量,表示当前有多少条目在使用 */
    __le16 eh_max;        /* 存储空间的最大容量,以条目数计 */
    __le16 eh_depth;      /* 树的深度,表示是否有实际的底层块 */
    __le32 eh_generation; /* 扩展索引树的版本号,用于确保一致性 */
};
```

- `eh_magic`:魔数,用于快速识别和验证数据结构的类型.在扩展索引中,这个字段通常包含一个特定的值,以帮助文件系统识别和处理扩展索引数据.
- `eh_entries`:当前索引树中有效条目的数量.这个值表示有多少个`ext4_extent`条目被用来存储文件的块映射信息.
- `eh_max`:索引树存储空间的最大容量,以条目数计.这个值定义了索引树可以存储的`ext4_extent`条目数量的最大值.
- `eh_depth`:索引树的深度.在ext4文件系统中,扩展索引可以形成一个树状结构,这个字段表示树的层次.如果一个索引树只有一个头节点而没有分支节点,那么它的深度为0;如果树有分支节点,则深度为1或更大.
- `eh_generation`:扩展索引树的版本号.这个字段用于跟踪索引树的变更,以确保在并发操作中保持数据的一致性.每当索引树发生变化时,这个值会增加,帮助系统检测到不一致的状态.

## ext 文件系统参数

||ext2|ext4|
|:--:|:--:|:--:|
|inode size(bytes)|128|256|
|logical block num(bit)|32|32|
|physical block num(bit)|32|48|
|max fs size|4GB|1EB|
|max file size|16GB|16TB|

> - ext2 max fs size = 1KB * 2^32 = 4GB
> - ext2 max file size = 1KB * (12 + 256 + 256x256 + 256x256x256) ~= 16GB
> - ext4 max fs size = 4KB * 2^48 = 1EB
> - ext4 max file size = 4KB * 2^32 = 16TB

## 特点

1. **更大的文件系统和文件支持**:ext4文件系统设计用于支持非常大的文件系统和文件大小.它可以支持高达1 Exbibyte的分区和最大16 Tebibyte的单个文件.

2. **Extents**:Ext4引入了Extents的概念,以替代Ext2和Ext3中使用的间接块映射模式.Extents是连续的物理块区域,可以显著提高对大文件操作的效率并减少文件碎片.

3. **向下兼容**:Ext4设计为向下兼容Ext3和Ext2,这意味着现有的Ext3和Ext2文件系统可以被挂载为Ext4分区.

4. **预留空间**:Ext4支持预先为文件保留磁盘空间的功能,这与使用`fallocate()`系统调用的其他文件系统如XFS类似.

5. **延迟获取空间**:Ext4采用了allocate-on-flush策略,这意味着它在数据实际需要写入磁盘时才开始分配空间,这有助于提高性能并减少文件碎片.

6. **突破子目录限制**:Ext4提高了子目录的数量限制,从Ext3的32000个增加到64000个,并且通过使用"dir_nlink"特性,这个限制可以进一步提高.

7. **日志校验和**:Ext4为日志数据添加了校验功能,这有助于检测日志数据是否损坏,并简化了从损坏日志中恢复数据的过程.

8. **支持"无日志"模式**:Ext4允许关闭日志功能,这可以在不需要日志保护的情况下进一步提升性能.

9. **在线磁盘整理**:Ext4支持在线碎片整理,并提供了e4defrag工具用于文件或整个文件系统的碎片整理.

10. **快速文件系统检查**:Ext4通过在inode中标记未使用的区块,可以加速文件系统检查过程,减少检查时间.

11. **多块分配**:Ext4改进了块分配算法,可以在一次操作中分配多个块,并尝试让这些块连续,从而提高处理大文件的性能.

12. **支持纳秒级时间戳**:Ext4引入了纳秒级时间戳,以满足对时间精度更高的需求,并扩展了时间戳的范围,避免了"2038年问题".

13. **项目配额**:Ext4支持为特定的项目ID设置磁盘配额限制.

14. **写Barrier**:Ext4默认启用写Barrier,确保即使在写缓存断电的情况下,文件系统元数据也能在磁盘上正确写入和排序.

[what are inode numbers 1 and 2 used for](https://stackoverflow.com/questions/24613454/what-are-inode-numbers-1-and-2-used-for)

[why do inode numbers start from 1 and not 0](https://stackoverflow.com/questions/2099121/why-do-inode-numbers-start-from-1-and-not-0/2109363#2109363)

[oracle blogs](https://blogs.oracle.com/linux/post/understanding-ext4-disk-layout-part-2)

## 参考

- [ext4 disk layout](https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout)
- [14.ext2文件系统](https://www.bilibili.com/video/BV1V84y1A7or/)
- [ext4-wiki](https://ext4.wiki.kernel.org/index.php/Main_Page) 文档
- [系统性学习Ext4文件系统(图例解析)](https://zhuanlan.zhihu.com/p/476377123)
- [opensource ext4-filesystem](https://opensource.com/article/18/4/ext4-filesystem)
- [日志式文件系统(ext3)详解](https://www.cnblogs.com/yuanqiangfei/p/16932969.html)
- [Linux 文件系统 EXT4 的前世今生](https://www.oschina.net/translate/introduction-ext4-filesystem)
- [linux虚拟文件系统(二)-ext4文件系统结构](https://blog.csdn.net/sinat_22338935/article/details/119270371)
- [漫谈Linux标准的文件系统(Ext2/Ext3/Ext4)](https://www.cnblogs.com/justmine/p/9128730.html)
- [干货!大话EXT4文件系统完整版](https://cloud.tencent.com/developer/article/1551286)
- [Linux ext2, ext3, ext4 文件系统解读[1]](https://blog.csdn.net/qwertyupoiuytr/article/details/70305582)
- [Linux ext2, ext3, ext4 文件系统解读[2]](https://blog.csdn.net/qwertyupoiuytr/article/details/70471623)
- [Linux ext2, ext3, ext4 文件系统解读[3]](https://blog.csdn.net/qwertyupoiuytr/article/details/70554469)
- [Linux ext2, ext3, ext4 文件系统解读[4]](https://blog.csdn.net/qwertyupoiuytr/article/details/70833690)
- [Linux ext2, ext3, ext4 文件系统解读[5]](https://blog.csdn.net/qwertyupoiuytr/article/details/70880547)
- [大话EXT4文件系统](http://www.ssdfans.com/?p=8136)
- [Ext4](https://en.wikipedia.org/wiki/Ext4)
- [Ext4#Features](https://en.wikipedia.org/wiki/Ext4#Features)
- [linux-wiki Ext4](http://linux-wiki.cn/wiki/Ext4)
- [baidu baike](https://baike.baidu.com/item/Ext4/1858450)