## How to compile nginx-1.4.0 on Debian 10  with our tool

## Virtual Machine
We provide a Vagrantfile for quickly set up a Debian 10 envoronmet.

1. Install dependency: VirtualBox and Vagrant
```
$ sudo apt install virtualbox
$ sudo apt install vagrant
```

2. Launch and login the virtual machine.
```
$ vagrant up
$ vagrant ssh
```

## Compile and run the nginx

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

