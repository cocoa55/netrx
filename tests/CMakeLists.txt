include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.8.1
  GIT_SHALLOW TRUE
  SYSTEM)
FetchContent_MakeAvailable(Catch2)

include(Catch)

set(NETRX_TEST_SANITIZE -fsanitize=address,undefined -fno-omit-frame-pointer
                        -fno-sanitize-recover=all)

# Sanitized copies of netrx_common and netrx_itch, used only by the tests.
# Same sources as the root CMakeLists.txt, so the normal targets keep
# their flags.
add_library(netrx_common_san ${CMAKE_SOURCE_DIR}/src/common/stats.cpp
                             ${CMAKE_SOURCE_DIR}/src/common/mappedfile.cpp)
target_include_directories(netrx_common_san PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(netrx_common_san PRIVATE -Wall -Wextra -Wpedantic
                                                ${NETRX_TEST_SANITIZE})
target_link_options(netrx_common_san PUBLIC ${NETRX_TEST_SANITIZE})

add_library(netrx_itch_san ${CMAKE_SOURCE_DIR}/src/itch/decoder.cpp
                           ${CMAKE_SOURCE_DIR}/src/itch/parser.cpp)
target_link_libraries(netrx_itch_san PUBLIC netrx_common_san)
target_compile_options(netrx_itch_san PRIVATE -Wall -Wextra -Wpedantic
                                              ${NETRX_TEST_SANITIZE})

add_executable(netrx_tests decoder_test.cpp parser_test.cpp)
target_link_libraries(netrx_tests PRIVATE netrx_itch_san Catch2::Catch2WithMain)
target_compile_options(netrx_tests PRIVATE -Wall -Wextra -Wpedantic
                                           ${NETRX_TEST_SANITIZE})
target_link_options(netrx_tests PRIVATE ${NETRX_TEST_SANITIZE})

catch_discover_tests(netrx_tests)
