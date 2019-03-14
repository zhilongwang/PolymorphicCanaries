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
If you used our code, please cite our paper.

```
To Detect Stack Buffer Overflow with Polymorphic Canaries

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

## Installation

### Compiler based PSSP
For program with source code.

#### Build Runtime Environment
~~~~{.sh}
# build runtime environment
$ cd /Runtime Environment
$ make
~~~~

#### Build Compiler Plugin
Make your choose according your needs.
~~~~{.sh}
# build LLVM pass 
$ cd Compiler based Implementation/P-SSP
$ mkdir build && cd build
$ cmake ..
$ make

# build gcc plugin(if your compiler is GCC
$ cd GCC_PLUGIN
$ make
~~~~

#### Compile your Program

##### GCC
~~~~{.sh}
# For small program, compile your application with the following (GNU GCC) flags: 
$ gcc -fstack-protector -fplugin=<PROJECT_SOURCE_DIR>/GCC_PLUGIN/PolymorphicCanaries.so test.c -o test

# For larger projects, adding `-fstack-protector',`-fno-omit-frame-pointer', and `-fplugin=<PROJECT_SOURCE_DIR>/GCC_PLUGIN/PolymorphicCanaries.so' to `CFLAGS'.
~~~~

##### LLVM
~~~~{.sh}
# For small program, compile your application with the following flags: 
$ clang -Xclang -load -Xclang <PROJECT_SOURCE_DIR>/Compiler based Implementation/P-SSP/libStackDoubleProtector.so test.c -o test


# For larger projects, adding `-Xclang -load -Xclang <PROJECT_SOURCE_DIR>/Compiler based Implementation/P-SSP/libStackDoubleProtector.so' to `CFLAGS'.
~~~~

#### Run your program with PSSP
~~~~{.sh}
# run 
$ export LD_PRELOAD=<PROJECT_SOURCE_DIR>/Runtime Environment/LIBPolymorphicCanaries.so
$ ./yourprogram
~~~~


### Binary rewriter
For program without source code. 

#### Build Instrumentor

Customize glibc.
<font size=3>
1. Download a version of [glibc](https://www.gnu.org/software/libc/) which is compatible with your OS.
2. Customize the stack_chk_fail.c file in glibc according the template in [/Binary based implementation/dynamic linked proram/stack_chk_fail.c](https://github.com/zhilongwang/PolymorphicCanaries/blob/master/Binary%20based%20implementation/dynamic%20linked%20proram/stack_chk_fail.c)
3. Build and install the modified glibc.
</font>


~~~~{.sh}
# Build instrumentor
$ cd Binary based implementation/dynamic linked proram/
$ make
~~~~   

~~~~{.sh}
# Rewrite your programs
$ ./Binary based implementation/dynamic linked proram/InstrumentationCode yourprogram
~~~~    

~~~~{.sh}
# Run your program with PSSP
$ export LIB_LIBRARY_PATH=<CUSTOMIZED_GLIBC_LIB_DIR>/*.so
$ export LD_PRELOAD=<PROJECT_SOURCE_DIR>/Runtime Environment/LIBPolymorphicCanaries.so
$ ./yourprogram
~~~~
