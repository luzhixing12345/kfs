#!/bin/bash

# 创建文件
touch testfile.txt

# 创建目录
mkdir testdir

# 移动文件
mv testfile.txt testdir/
if [ -f testdir/testfile.txt ]; then
    echo "文件移动成功"
else
    echo "文件移动失败"
fi

# 清理
rm testdir/testfile.txt
rmdir testdir
