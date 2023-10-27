cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # For clang-tidy.
set(BUILD_SHARED_LIBS OFF)              # We expect external libraries to be linked statically.
set(CMAKE_CXX_STANDARD 17)              # Compile as C++17.
set(CMAKE_CXX_STANDARD_REQUIRED ON)     # Require C++17 support.
set(CMAKE_BUILD_TYPE "Debug")
SET(ROOT_DIR ${CMAKE_SOURCE_DIR})
# 设置c的编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -Wall -ggdb")
# set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wno-unused-parameter -Wno-attributes") #TODO: remove
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -ggdb -fno-omit-frame-pointer -fno-optimize-sibling-calls")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(${ROOT_DIR}/src)
include_directories(${ROOT_DIR}/src/include)
include_directories(${ROOT_DIR}/test)
include_directories(${ROOT_DIR})

LINK_DIRECTORIES(${ROOT_DIR}/src)
LINK_DIRECTORIES(${ROOT_DIR}/test)
LINK_DIRECTORIES(${ROOT_DIR}/src/include)
LINK_DIRECTORIES(${ROOT_DIR})
# 设置项目名
project(kv_store) 

add_subdirectory(src)
# add_subdirectory(third_party)
add_subdirectory(test)