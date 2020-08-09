cmake_minimum_required(VERSION 3.10)
project(mymuduo)

# mymuduo 最终的生成的动态库路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)

# 定义参与编译的源文件
aux_source_directory(. SRC_LIST)

# 编译生成动态库
add_library(mymuduo SHARED ${SRC_LIST})

