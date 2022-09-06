cmake_minimum_required(VERSION 3.23)

project(clionbuild)

set(CMAKE_CXX_STANDARD 14)

aux_source_directory(../src DIR_SRC)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../dev)

include_directories(../src)
#add_subdirectory(../src)

add_executable(SmartRouter ${DIR_SRC})
