# How to compile nginx-1.4.0 on Debian 10  with our tool


1. Firstly, run
```
$ wget http://nginx.org/download/nginx-1.4.0.tar.gz
$ tar -xvf nginx-1.4.0.tar.gz
$ cd nginx-1.4.0/
$ ./configure
```

2. Secondly, adding
```
-fstack-protector-all -fplugin=/home/vagrant/PolymorphicCanaries/GCC_PLUGIN/PolymorphicCanaries.so
```
to 'CFLAGS' in objs/Makefile

3. Thirdly, run
```
make -f objs/Makefile -j16
```

4. Finally, run the program

```
LD_PRELOAD=/home/vagrant/PolymorphicCanaries/Runtime_Environment/Binary_Based_Version/LIBPolymorphicCanaries.so ./objs/nginx
```

