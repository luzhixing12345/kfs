
sudo mount disk.img tmp
cd tmp

mkdir a
mkdir b
echo "hello world" > a/file.txt
cd ..

sudo umount tmp