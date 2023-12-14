cmake_minimum_required(VERSION 3.26)
project(PA4)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

add_executable(PA4
        main.cpp
)

target_link_libraries(PA4 Threads::Threads)