
cmake_minimum_required (VERSION 3.18)
project (hust)

include_directories(hgs)
include_directories(smart)

add_subdirectory(hgs)
add_subdirectory(smart)

aux_source_directory(. ALL_SRCS)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../dev")
add_executable(SmartRouter ${ALL_SRCS})
target_link_libraries(SmartRouter hgs smart)