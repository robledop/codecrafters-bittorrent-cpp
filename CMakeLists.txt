cmake_minimum_required(VERSION 3.13)

project(bittorrent-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard

add_executable(bittorrent ${SOURCE_FILES}
        src/info.cpp
        src/info.h
        src/lib/sha1.hpp
        src/torrent.cpp
        src/torrent.h)
