#!/bin/bash

# 创建文件并写入内容
echo "Hello, world!" > testfile.txt

# 复制文件
cp testfile.txt copyfile.txt
if [ -f copyfile.txt ]; then
    echo "文件复制成功"
else
    echo "文件复制失败"
    exit -1
fi

# 检查内容是否一致
original_content=$(cat testfile.txt)
copy_content=$(cat copyfile.txt)
if [ "$original_content" == "$copy_content" ]; then
    echo "文件内容一致"
else
    echo "文件内容不一致"
    exit -1
fi

# 清理
rm testfile.txt copyfile.txt

#!/bin/bash

# Create a directory to copy from
mkdir copy_from_dir
if [ $? -ne 0 ]; then
    echo "Failed to create directory copy_from_dir"
    exit -1
fi

# Create files to copy
touch copy_from_dir/file1.txt
touch copy_from_dir/file2.txt
if [ $? -ne 0 ]; then
    echo "Failed to create files in copy_from_dir"
    exit -1
fi

# Create a directory to copy to
mkdir copy_to_dir
if [ $? -ne 0 ]; then
    echo "Failed to create directory copy_to_dir"
    exit -1
fi

# Copy files
cp copy_from_dir/file1.txt copy_to_dir/
cp copy_from_dir/file2.txt copy_to_dir/
if [ $? -ne 0 ]; then
    echo "Failed to copy files"
    exit -1
fi

echo "Files copied successfully"

# Cleanup
rm -rf copy_from_dir copy_to_dir
