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
*fibonacci.cpp* : llvm original example \
*jit.cpp* : learn mangle and demangle \
*jit_example.cpp* : how to use RTDyldMemoryManager \
*bitcode_example.cpp* : how to read bitcode and execute by ExecutionEngine \
*pass_example.cpp* : how to use PassManager \
*runtimedyld_example.cpp* : how to compile Module to ObjectFile and use runtimeDyld to getSymbol \
*orcjit* : how to use orcjit
