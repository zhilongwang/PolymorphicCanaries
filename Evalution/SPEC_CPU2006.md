# 0x00 背景  
最近在帮师兄做静态插桩，然后插桩的工作做得差不多了，需要用到benchmark做性能测试，简单记录一下`SPEC CPU2006`的安装和使用过程。中间也遇到了许多坑，谨记。  

# 0x01 安装
机器版本：Ubuntu 16.04.3 LTS  
首先下载CPU2006到本地，一般会下载到镜像文件，解压之后可以找到`install.sh`
```sh
~$ ./install.sh
SPEC CPU2006 Installation

Top of the CPU2006 tree is '/media/*/SPEC_CPU2006v1.1'
Enter the directory you wish to install to (e.g. /usr/cpu2006)
/home/myname/cpu2006

Installing FROM /media/*/SPEC_CPU2006v1.1
Installing TO /home/myname/cpu2006

Is this correct? (Please enter 'yes' or 'no')
yes
...
```
之后等待安装完成，到此安装过程结束。  

# 0x02 编译工具包
根据文档，在使用之前需要先配置环境。令人欣慰的是在CPU2006中已经集成了配置环境的方法，只需要执行一条命令就可以完成配置操作  
```sh
source ./shrc
```
每次重新打开terminal的时候，都需要先运行该命令，之后才可以正常使用。  

在编译工具之前，我们首先需要进入config文件夹，根据自己的需要，复制一份已有的配置文件模板作为自己使用的文件，例如
```sh
cp Example-linux64-amd64-gcc43.cfg my.cfg
```
例如我们只需要测试int型的CPU性能，我们可以执行以下操作：
```sh
runspec --config=my.config -T base --action=build int
```
在编译生成gcc的过程中遇到一些问题，查阅资料发现是因为编译CPU2006需要gcc-4.9及以下的版本才可以编译，因此需要安装低版本的gcc。可以通过`apt-get`来安装。
```sh
sudo apt-get install 4.9
```
安装完成之后，可以通过下面的命令实现gcc版本切换
```sh
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 50
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.9 40
```
接着，输入以下命令可以切换版本
```sh
sudo update-alternatives --config gcc
There are 2 choices for the alternative gcc (providing /usr/bin/gcc).

  Selection    Path              Priority   Status
------------------------------------------------------------
  0            /usr/bin/gcc-5     100       auto mode
* 1            /usr/bin/gcc-4.9   50        manual mode
  2            /usr/bin/gcc-5     100       manual mode

Press <enter> to keep the current choice[*], or type selection number: 
```
如果有遇到g++也需要不同版本的问题，同样使用该办法来实现g++的切换。

安装完成之后，如果没有问题，重新执行编译命令，便可以编译完成。

# 0x03 运行
如果一切没有问题，接下来可以进行运行测试
```sh
runspec --config=my.cfg --T base --reportable int
```
如果需要生成报告，一定要写`--reportable`参数，运行该命令时，程序会自动检测编译生成的二进制文件有没有修改过，如果原来生成的二进制文件被修改了，则运行时会自动重新编译生成二进制文件，确保运行的程序是原始的程序。

# 0x04 运行插桩过后的程序
因为运行时会检测程序是否是编译结束的程序，因此，就存在一个问题，插桩过的程序一定被修改了，如何运行，查阅了一些资料发现：  
- 程序运行之前做的检测只是对比可执行程序的 MD5
- 可执行程序的 MD5 值在 my.config 文件中保存  

因此，只需要在插桩后将可执行程序新的 MD5 写入 my.config 文件中，就不会出现重新编译的情况，这也算是一种投机取巧的办法。可以写一个小脚本自动来填写MD5值。  

当然，如果不需要生成报告，也可以在运行时加入参数`--nobuild`  
配置文件中需要加的参数，程序的运行环境`LD_PRELOAD`
```sh
preENV_LD_PRELOAD=lib/libredundantguard.so
```

# 0x05 总结
搞了有几天的spec CPU2006，也只是知道了一点点的东西，记录下来，以免以后会忘记。

