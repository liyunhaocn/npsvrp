
cmake_minimum_required(VERSION 3.18)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
aux_source_directory(. LIB_SRCS)			# 添加当前目录下所有的源文件
add_library (hgs ${LIB_SRCS})				# 当前目录下的文件生成一个链接库

#file(GLOB_RECURSE CPP_FILES ./*.cpp)
#add_library(hgs SHARED ${CPP_FILES})

