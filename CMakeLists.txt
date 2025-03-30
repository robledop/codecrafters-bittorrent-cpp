cmake_minimum_required(VERSION 3.13)
set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

project(bittorrent-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

set(CMAKE_CXX_STANDARD 23) 

#include(FetchContent)
#FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
#        GIT_TAG dd967cb48ea6bcbad9f1da5ada0db8ac0d532c06) # Replace with your desired git commit from: https://github.com/libcpr/cpr/releases
#FetchContent_MakeAvailable(cpr)

find_package(cpr CONFIG REQUIRED)

add_executable(bittorrent ${SOURCE_FILES}
        src/info.cpp
        src/info.h
        src/lib/sha1.hpp
        src/torrent.cpp
        src/torrent.h
        src/b_decoder.cpp
        src/b_decoder.h
        src/util.h
        src/peers.cpp
        src/peers.h)

target_link_libraries(bittorrent PRIVATE cpr::cpr)