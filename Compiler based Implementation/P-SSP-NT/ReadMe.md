# Implementation of P-SSP-NT in LLVM

## When we have the .ll files after the clang tool, we need the llc tool to produce the binary file and add the attribute -mattr=+rdrnd to generate x86's rdrand instruction.

``` llc -filetype=obj input.ll -mattr=+rdrnd -o output.o```
