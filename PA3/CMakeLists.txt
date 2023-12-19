cmake_minimum_required(VERSION 3.22.1)
project(PA3)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

add_executable(PA3
        rideshare.cpp
        rideshare2.cpp
)

target_link_libraries(PA3 Threads::Threads)