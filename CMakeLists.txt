cmake_minimum_required(VERSION 3.12)
project(llvm_jit)

add_subdirectory(llvm-project/llvm)

message(STATUS "BINARY output: " ${PROJECT_BINARY_DIR})
message(STATUS "SOURCE output: " ${PROJECT_SOURCE_DIR})
message(STATUS "CMAKE CURRENT SOURCE DIR: " ${CMAKE_CURRENT_SOURCE_DIR})
set(LLVM_DIR ${PROJECT_BINARY_DIR}/llvm-project/llvm/lib/cmake/llvm/)
message(STATUS "BINARY output: " ${LLVM_DIR})
find_package(LLVM REQUIRED CONFIG HINTS "${LLVM_DIR}")

#SET(CMAKE_CXX_COMPILER "/usr/bin/clang++")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
set(NO_RTTI "-fno-rtti")
add_definitions(${NO_RTTI})
add_definitions(-DLLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING=1)
add_executable(bitcode_example bitcode_example.cpp)
add_executable(pass_example pass_example.cpp)
add_executable(runtimedyld_example runtimedyld_example.cpp)
add_executable(jit_example jit_example.cpp)
add_executable(llvm_example fibonacci.cpp)
add_executable(jit_example2 jit.cpp)
llvm_map_components_to_libnames(llvm_libs all support core irreader executionengine mc mcjit interpreter nativecodegen ipo x86asmparser native)

add_custom_command(TARGET bitcode_example
		    PRE_BUILD
		    COMMAND clang++ -O1 -emit-llvm ${CMAKE_CURRENT_SOURCE_DIR}/c_fib.cpp -c -o cfib.bc 
		    VERBATIM
		    )
#add_custom_command(TARGET jit_cmake
#		    POST_BUILD
#		    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cfib.bc ${CMAKE_CURRENT_SOURCE_DIR}/build 
#		    VERBATIM
#		    )

target_link_libraries(bitcode_example PUBLIC ${llvm_libs})
target_link_libraries(pass_example PUBLIC ${llvm_libs})
target_link_libraries(runtimedyld_example PUBLIC ${llvm_libs})
target_link_libraries(jit_example PUBLIC ${llvm_libs})
target_link_libraries(jit_example2 PUBLIC ${llvm_libs})
target_link_libraries(llvm_example PUBLIC ${llvm_libs})
