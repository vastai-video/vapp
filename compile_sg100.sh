export CC=gcc
export PTHREAD_LIB=-lpthread

mkdir -p lib
mkdir -p bin
cd ./src
make clean
make debug
make install
cd ../test
make clean
make debug
make install
cd ..
cp ./lib/libvappi.so /opt/vastai/vaststream/lib
mkdir -p /opt/vastai/vaststream/lib/op/ext_op/video/
cp ./elf_sg100/* /opt/vastai/vaststream/lib/op/ext_op/video/

