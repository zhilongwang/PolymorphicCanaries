# Implementation of P-SSP-NT in LLVM


## how to compile a program.
```clang-6.0 -fno-stack-protector -O1 -Xclang -load -Xclang path/to/libSSPPassNT.so -mrdrnd test.c -o test```
