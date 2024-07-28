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
    exit -1
fi

# 清理
rm testdir/testfile.txt
rmdir testdir

#!/bin/bash

# Create a directory move from and to
mkdir move_from_dir && mkdir move_to_dir

# Create files to move
touch move_from_dir/file1.txt
touch move_from_dir/file2.txt
if [ $? -ne 0 ]; then
    echo "Failed to create files in move_from_dir"
    exit -1
fi

# Move files
mv move_from_dir/file1.txt move_to_dir/
mv move_from_dir/file2.txt move_to_dir/
if [ $? -ne 0 ]; then
    echo "Failed to move files"
    exit -1
fi

echo "Files moved successfully"

# Cleanup
rm -rf move_from_dir move_to_dir
