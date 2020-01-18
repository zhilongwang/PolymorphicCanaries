# Implementation of P-SSP-NT in LLVM

## Environment 

- **Operation System:** Ubuntu 16.04
- **LLVM Version:** llvm 6.0


## How to compile a program.
```clang-6.0 -fno-stack-protector -O1 -Xclang -load -Xclang path/to/libSSPPassNT.so -mrdrnd test.c -o test```
