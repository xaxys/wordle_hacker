cmake_minimum_required(VERSION 3.10)
project(wordle_hacker)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_subdirectory(third_party/fmt)
add_executable(wordle_hacker main.cpp guesser.cpp guesser.h)
target_link_libraries(wordle_hacker fmt::fmt)
