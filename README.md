# llvm_example

llvm example in llvm13

### How to use llvm_example
1. 下载代码：\
git clone git@github.com:awakeljw/llvm_example.git \
git submodule update --init --recursive \

2. 编译
cmake -B build \
cmake --build build -j 48

3. 运行
cd build
./jit_cmake
./jit2_cmake
