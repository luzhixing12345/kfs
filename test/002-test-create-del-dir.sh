#!/bin/bash

# 创建目录
mkdir testdir
if [ -d testdir ]; then
    echo "目录创建成功"
else
    echo "目录创建失败"
    exit -1
fi

# 删除目录
rmdir testdir
if [ ! -d testdir ]; then
    echo "目录删除成功"
else
    echo "目录删除失败"
    exit -1
fi
