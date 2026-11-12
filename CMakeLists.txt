cmake_minimum_required(VERSION 3.20)
project(netrx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Shared code
add_library(netrx_common src/common/stats.cpp
        src/common/mappedfile.cpp)
target_include_directories(netrx_common PUBLIC include)
target_compile_options(netrx_common PRIVATE -Wall -Wextra -Wpedantic)

# ITCH decoding
add_library(netrx_itch src/itch/decoder.cpp)
target_link_libraries(netrx_itch PUBLIC netrx_common)
target_compile_options(netrx_itch PRIVATE -Wall -Wextra -Wpedantic)

# Offline ITCH dump tool
add_executable(itchdump src/tools/itchdump/main.cpp)
target_link_libraries(itchdump PRIVATE netrx_itch)
target_compile_options(itchdump PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(itchdump PRIVATE
        NETRX_ITCH_FILE="${CMAKE_SOURCE_DIR}/planning/20191230.BX_ITCH_50")

# Tests
enable_testing()
add_executable(itchdump_addorder_test src/tools/itchdump/addorder_test.cpp)
target_link_libraries(itchdump_addorder_test PRIVATE netrx_itch)
target_compile_options(itchdump_addorder_test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME itch_add_order COMMAND itchdump_addorder_test)
