# To-Detect-Stack-Buffer-Overflow-With-Polymorphic-Canaries.  
=======================================================.  
A High Efficient Protection against Brute-force Attacks
=======================================================. 


Implementation of Polymorphic Canaries.


## Installation

### Compile your source code
~~~~{.sh}
# compile llvm pass
$ cd Compiler based Implementation/P-SSP/
$ make

# install z3 and system deps
$ ./setup.sh

# install using virtual env
$ virtualenv venv
$ source venv/bin/activate
$ pip install .
~~~~

### Rewrite your binary file

## Authors
- Zhilong Wang <mg1633081@smail.nju.edu.cn>
- Xuhua Ding <xhding@smu.edu.sg>
- Chengbin Pang <mg1733051@smail.nju.edu.cn>
- Jian Guo <mg1733051@smail.nju.edu.cn>
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
