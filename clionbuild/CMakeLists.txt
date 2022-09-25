cmake_minimum_required(VERSION 3.23)

project(clionbuild)

set(CMAKE_CXX_STANDARD 14)

aux_source_directory(../src DIR_SRC)
aux_source_directory(../src/hgs DIR_SRC)
aux_source_directory(../src/smart DIR_SRC)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../dev)


include_directories(../src)
include_directories(../src/hgs)
include_directories(../src/smart)

add_executable(SmartRouter ${DIR_SRC})
