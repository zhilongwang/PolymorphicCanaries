# P-SSP is the basic scheme implementation in LLVM pass.


## Environment 

- **Operation System:** Ubuntu 16.04
- **LLVM Version:** llvm 6.0

## how to compile a test.

``` clang-6.0 -fno-stack-protector -O0 -Xclang -load -Xclang /path/to/libSSPPass.so test.c -o test ```
