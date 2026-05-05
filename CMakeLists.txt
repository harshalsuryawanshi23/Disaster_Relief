ccmake_minimum_required(VERSION 3.15)
project(disaster_relief)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
    cpp/ds
    cpp/core
    cpp/output
)

set(SOURCES
    cpp/main.cpp
    cpp/core/DataLoader.cpp
    cpp/core/Graph.cpp
    cpp/core/Router.cpp
    cpp/output/StateExporter.cpp
    cpp/ds/FibonacciHeap.cpp
    cpp/ds/MinHeap.cpp
    cpp/ds/QuadTree.cpp
    cpp/ds/SegmentTree.cpp
    cpp/ds/SkipList.cpp
    cpp/ds/Trie.cpp
    cpp/ds/UnionFind.cpp
)

add_executable(disaster_relief ${SOURCES})
