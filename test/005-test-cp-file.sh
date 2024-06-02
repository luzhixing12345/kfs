#!/bin/bash

# 创建文件并写入内容
echo "Hello, world!" > testfile.txt

# 复制文件
cp testfile.txt copyfile.txt
if [ -f copyfile.txt ]; then
    echo "文件复制成功"
else
    echo "文件复制失败"
fi

# 检查内容是否一致
original_content=$(cat testfile.txt)
copy_content=$(cat copyfile.txt)
if [ "$original_content" == "$copy_content" ]; then
    echo "文件内容一致"
else
    echo "文件内容不一致"
fi

# 清理
rm testfile.txt copyfile.txt
