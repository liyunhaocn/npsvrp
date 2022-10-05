cmake_minimum_required(VERSION 3.18)

project(hust)

set(CMAKE_CXX_STANDARD 14)

#add_subdirectory(dev)
#add_subdirectory(src/smart)
#add_subdirectory(src/hgs)

aux_source_directory(src DIR_SRC)
aux_source_directory(src/hgs DIR_SRC)
aux_source_directory(src/smart DIR_SRC)

include_directories(src)
include_directories(src/hgs)
include_directories(src/smart)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dev")

add_executable(SmartRouter ${DIR_SRC})
