cmake_minimum_required(VERSION 3.20)
project(netrx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Shared code
add_library(netrx_common src/common/stats.cpp)
target_include_directories(netrx_common PUBLIC include)
target_compile_options(netrx_common PRIVATE -Wall -Wextra -Wpedantic)
