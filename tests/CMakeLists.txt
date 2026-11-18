include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.8.1
  GIT_SHALLOW TRUE
  SYSTEM)
FetchContent_MakeAvailable(Catch2)

include(Catch)

# Sanitizers come from the build configuration (NETRX_ENABLE_SANITIZERS,
# on by default for Debug), so the tests link the regular libraries.
add_executable(netrx_tests decoder_test.cpp parser_test.cpp orderbook_test.cpp
               refmap_test.cpp)
target_include_directories(netrx_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/orderbook)
target_link_libraries(netrx_tests PRIVATE netrx_itch Catch2::Catch2WithMain)
netrx_target_defaults(netrx_tests)

catch_discover_tests(netrx_tests)

# constexpr tests: STATIC_REQUIRE fails the build if a check is not a
# constant expression or is false.
add_executable(netrx_constexpr_tests constexpr_test.cpp)
target_link_libraries(netrx_constexpr_tests PRIVATE netrx_common
                      Catch2::Catch2WithMain)
netrx_target_defaults(netrx_constexpr_tests)
catch_discover_tests(netrx_constexpr_tests)

# Same file at runtime, so a failing check can be stepped through.
add_executable(netrx_relaxed_constexpr_tests constexpr_test.cpp)
target_compile_definitions(netrx_relaxed_constexpr_tests PRIVATE
                           CATCH_CONFIG_RUNTIME_STATIC_REQUIRE)
target_link_libraries(netrx_relaxed_constexpr_tests PRIVATE netrx_common
                      Catch2::Catch2WithMain)
netrx_target_defaults(netrx_relaxed_constexpr_tests)
catch_discover_tests(netrx_relaxed_constexpr_tests)
