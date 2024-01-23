# LLVM Examples

llvm examples build with llvm13. It is suitable for beginners of LLVM.

### How to use llvm_example
1. 下载代码：\
git clone git@github.com:awakeljw/llvm_example.git \
git submodule update --init --recursive 

2. 编译
cmake -B build \
cmake --build build

### Examples
bitcode example: how to read bitcode and execute by executionengine
pass example: how to use passManager
runtimedyld example: how to compile Module to ObjectFile and use runtimeDyld to getSymbol
