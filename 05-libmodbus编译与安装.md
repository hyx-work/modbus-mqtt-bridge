## 0 编译C、C++代码前置依赖

yum install -y  gcc gcc-c++  kernel-devel kernel-headers

yum install -y autoconf automake libtool

## 1 编译安装libmodbus

make & make install

## 2 动态库软链接

ln -s /usr/local/lib/libmodbus.so.5  /usr/lib/libmodbus.so.5

ldconfig