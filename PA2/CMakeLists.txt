cmake_minimum_required(VERSION 3.26)
project(PA2)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

add_executable(PA2
        cli.cpp
)

target_link_libraries(PA2 Threads::Threads)