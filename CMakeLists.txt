cmake_minimum_required(VERSION 3.20)
project(netrx LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(receiver src/receiver/main.cpp)
target_include_directories(receiver PRIVATE include)
target_compile_options(receiver PRIVATE -Wall -Wextra -Wpedantic)
