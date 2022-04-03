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

add_definitions(-DLLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING=1)
add_executable(jit_cmake fibonacci_jit.cpp)
add_executable(jit2_cmake fibonacci_jit2.cpp)
llvm_map_components_to_libnames(llvm_libs all support core irreader executionengine mc mcjit interpreter nativecodegen ipo x86asmparser native)

add_custom_command(TARGET jit_cmake
		    PRE_BUILD
		    COMMAND clang++ -O1 -emit-llvm ${CMAKE_CURRENT_SOURCE_DIR}/c_fib.cpp -c -o cfib.bc 
		    VERBATIM
		    )
#add_custom_command(TARGET jit_cmake
#		    POST_BUILD
#		    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cfib.bc ${CMAKE_CURRENT_SOURCE_DIR}/build 
#		    VERBATIM
#		    )


target_link_libraries(jit_cmake PUBLIC ${llvm_libs})
target_link_libraries(jit2_cmake PUBLIC ${llvm_libs})
