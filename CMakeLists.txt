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

# Sender
add_executable(sender src/sender/main.cpp)
target_link_libraries(sender PRIVATE netrx_common)
target_compile_options(sender PRIVATE -Wall -Wextra -Wpedantic)

# Receiver
add_executable(receiver src/receiver/main.cpp)
target_link_libraries(receiver PRIVATE netrx_common)
target_compile_options(receiver PRIVATE -Wall -Wextra -Wpedantic)
