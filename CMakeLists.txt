cmake_minimum_required(VERSION 3.20)
project(netrx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Debug: -O0 -g, assert() active (no NDEBUG), plus libstdc++ bounds/precondition
# checks. Release: -O3 -DNDEBUG (CMake's defaults), so assert() compiles out.
add_compile_definitions($<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>)

# Warnings (-Wall -Wextra -Wconversion -Wshadow ... or /W4), clang-tidy and
# ASan/UBSan. See cmake/NetrxBuildOptions.cmake for defaults and switches.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(NetrxBuildOptions)

# Shared code
add_library(netrx_common src/common/stats.cpp
        src/common/mappedfile.cpp
        src/orderbook/orderbook.cpp
        include/netrx/orderbook.hpp
        include/netrx/refmap.hpp)
target_include_directories(netrx_common PUBLIC include)
netrx_target_defaults(netrx_common)

# ITCH decoding
add_library(netrx_itch src/itch/decoder.cpp src/itch/parser.cpp
        src/orderbook/orderbook.cpp
        include/netrx/orderbook.hpp
        include/netrx/refmap.hpp)
target_link_libraries(netrx_itch PUBLIC netrx_common)
netrx_target_defaults(netrx_itch)

# Offline ITCH dump tool
add_executable(itchdump src/tools/itchdump/main.cpp
        src/orderbook/orderbook.cpp
        include/netrx/orderbook.hpp
        src/feedhandler/feedhandler.cpp
        src/feedhandler/feedhandler.hpp
        include/netrx/refmap.hpp)
target_link_libraries(itchdump PRIVATE netrx_itch)
netrx_target_defaults(itchdump)
target_compile_definitions(itchdump PRIVATE
        NETRX_ITCH_FILE="${CMAKE_SOURCE_DIR}/planning/20191230.BX_ITCH_50")

# Tests
enable_testing()
add_executable(itchdump_addorder_test src/tools/itchdump/addorder_test.cpp)
target_link_libraries(itchdump_addorder_test PRIVATE netrx_itch)
netrx_target_defaults(itchdump_addorder_test)
add_test(NAME itch_add_order COMMAND itchdump_addorder_test)

add_subdirectory(tests)
