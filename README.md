# To-Detect-Stack-Buffer-Overflow-With-Polymorphic-Canaries.  
=======================================================.  
A High Efficient Protection against Brute-force Attacks
=======================================================. 

## Authors
- Zhilong Wang <mg1633081@smail.nju.edu.cn>
- Xuhua Ding <xhding@smu.edu.sg>
- Chengbin Pang <mg1733051@smail.nju.edu.cn>
- Jian Guo <mf1733018@smail.nju.edu.cn>
- Jun Zhu <clearscreen@163.com>
- Bing Mao <maobing@nju.edu.cn>


## Publications
If you use our code in your research, please cite our paper.

```
@inproceedings{polymorphiccanaries,
  author = {Z. Wang and X. Ding and C. Pang and J. Guo and J. Zhu and B. Mao},
  booktitle = {2018 48th Annual IEEE/IFIP International Conference on Dependable Systems and Networks (DSN)},
  title = {To Detect Stack Buffer Overflow with Polymorphic Canaries},
  year = {2018},
  volume = {00},
  number = {},
  pages = {243-254},
  keywords={Security;Runtime;Instruments;Force;Tools;Instruction sets},
  doi = {10.1109/DSN.2018.00035},
  url = {doi.ieeecomputersociety.org/10.1109/DSN.2018.00035},
  ISSN = {2158-3927},
  month={Jun}
}
```
## GCC Version
The GCC version is tested on Debian 10 with gcc-4.9/g++4.9.

### Install Dependency (gcc/g++, gcc-<version>-plugin-dev, build-essential)

1. add follow source to /etc/apt/sources.list
```
deb http://ftp.us.debian.org/debian/ jessie main contrib non-free
deb-src http://ftp.us.debian.org/debian/ jessie main contrib 
```

2. add key and update
```
$ sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 7638D0442B90D010
$ sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys CBF8D6FD518E17E1
$ sudo apt-get update
```

3. install gcc/g++, build-essential and gcc-<version>-plugin-dev
```
$ 
$ sudo apt install gcc-4.9 g++-4.9 gcc-4.9-plugin-dev build-essential
```

### Build Plugin and Library 
1. build gcc plugin
```
$ cd GCC_PLUGIN
$ make
```
2. build runtime library
```
$ cd Runtime_Environment/Binary_Based_Version/
$ make
```

### Compile Your Program or Project

1. For single file program, compile your application with the following (GNU GCC) flags: 
```
$ gcc -fstack-protector-all -fplugin=<PROJECT_SOURCE_DIR>/GCC_PLUGIN/PolymorphicCanaries.so demo.c -o demo
```

2. For large projects, adding 
```
-fstack-protector-all -fplugin=<PROJECT_SOURCE_DIR>/GCC_PLUGIN/PolymorphicCanaries.so
```
to 'CFLAGS' or 'CXXFLAGS' through configure, makefile, or Cmakefile.

### Run Compiled Program
```
LD_PRELOAD=<PROJECT_SOURCE_DIR>/Runtime_Environment/Binary_Based_Version/LIBPolymorphicCanaries.so ./demo
```


## LLVM Version
LLVM Version is tested on llvm-6.0

### Install Dependency (LLVM, Clang) and 
1. install llvm and clang
```
# sudo apt-get install llvm-6.0 clang-6.0
```

### Build LLVM Pass and and Library 

1. build LLVM Pass 
```
$ cd Compiler_based_Implementation/P-SSP
$ mkdir build && cd build
$ cmake ..
$ make
```

2. Build Runtime Environment
~~~~{.sh}
# build runtime environment
$ cd /Runtime_Environment/Compiler_Based_Version/
$ make
~~~~

### Compile Your Program or Project
1. For small program, compile your application with the following flags
```
$ clang -Xclang -load -Xclang <PROJECT_SOURCE_DIR>/Compiler_based_Implementation/P-SSP/libStackDoubleProtector.so demo.c -o demo
```


2. For larger projects, adding 
```
-Xclang -load -Xclang <PROJECT_SOURCE_DIR>/Compiler_based_Implementation/P-SSP/libStackDoubleProtector.so
```
to `CFLAGS' or `CXXFLAGS' through configure, makefile, or Cmakefile.

### Run Compiled Program
```
LD_PRELOAD=<PROJECT_SOURCE_DIR>/Runtime_Environment/Compiler_Based_Version/LIBPolymorphicCanaries.so ./demo
```


## Binary Rewriter Version


### Dynamic Linked Binary

1 Build Instrumentor
```
$ cd Binary_based_implementation/dynamic linked proram/
$ make
```

2. Build and install the customized GLIBC

- Download a version of [glibc](https://www.gnu.org/software/libc/) which is compatible with your OS.
- Replace the stack_chk_fail.c in GLIBC with a customized version [stack_chk_fail.c](https://github.com/zhilongwang/PolymorphicCanaries/blob/master/Binary_based_implementation/dynamic%20linked%20proram/stack_chk_fail.c).
- Build and install the modified glibc.

3. Rewrite your programs
```
$ ./Binary_based_implementation/dynamic linked proram/InstrumentationCode ./demo
```  

4. Run your program with PSSP
```
$ export LIB_LIBRARY_PATH=<CUSTOMIZED_GLIBC_LIB_DIR>/*.so
$ export LD_PRELOAD=<PROJECT_SOURCE_DIR>/Runtime_Environment/Binary_Based_Version/LIBPolymorphicCanaries.so
$ ./demo
```

### Static Linked Binary

We provide a binary rewriter based on [Dyninst](http://www.umiacs.umd.edu/mc2symposium/slides/securityTutorialDyninst.pdf) for static linked programs. The implementation is located at [Dyninst Tool](https://github.com/zhilongwang/PolymorphicCanaries/tree/master/Binary_based_implementation/static%20linked%20program).

