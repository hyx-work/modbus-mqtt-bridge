## 0 编译C、C++代码前置依赖

yum install -y  gcc gcc-c++  kernel-devel kernel-headers

yum install -y autoconf automake libtool

## 1 安装所需依赖

 yum install -y openssl-devel c-ares-devel e2fsprogs-devel uuid-devel libuuid-devel 

## 2 编译安装mosquitto （mqtt broker）

make & make install

## 3 动态库软链接

ln -s /usr/local/lib/libmosquitto.so.1 /usr/lib/libmosquitto.so.1

ldconfig

## 4 运行

adduser mosquitto

mosquitto -v

