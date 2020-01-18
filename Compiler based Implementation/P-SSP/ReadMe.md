# P-SSP is the basic scheme implementation in LLVM pass.

## how to compile a test.

``` clang-6.0 -fno-stack-protector -O0 -Xclang -load -Xclang /path/to/libSSPPass.so test.c -o test ```
