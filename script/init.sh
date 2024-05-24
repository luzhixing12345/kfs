
mount disk.img tmp
cd tmp
mkdir a
mkdir b
mkdir c
echo "hello world" > a/file.txt
mkdir c/a
echo "nice" > c/a/a.c
cd ..
umount tmp