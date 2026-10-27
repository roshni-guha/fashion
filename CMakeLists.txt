cmake_minimum_required(VERSION 3.20)
project(fashion_dist_engine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

# Common include path
include_directories(include)

# Target 1: Storage Node Server
add_executable(node_server
    src/storage/node_server.cpp
)
target_link_libraries(node_server PRIVATE Threads::Threads)

# Target 2: Edge Router Gateway (depends on hash_ring)
add_executable(edge_router
    src/storage/edge_router.cpp
    src/hash_ring/hash_ring.cpp
)
target_link_libraries(edge_router PRIVATE Threads::Threads)