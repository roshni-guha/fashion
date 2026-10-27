cmake_minimum_required(VERSION 3.16)
project(fashion CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(test_ring src/main.cpp src/hash_ring/hash_ring.cpp)
target_include_directories(test_ring PRIVATE src)
