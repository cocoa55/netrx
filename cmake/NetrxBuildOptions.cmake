# Project-wide build hygiene: warnings, clang-tidy, sanitizers.
#
# Defaults: Debug builds get sanitizers + clang-tidy, everything else gets
# neither. Each can be overridden at configure time, e.g.
#   cmake --preset release -DNETRX_ENABLE_CLANG_TIDY=ON

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(_netrx_debug_default ON)
else()
  set(_netrx_debug_default OFF)
endif()

option(NETRX_ENABLE_SANITIZERS "Build with ASan + UBSan" ${_netrx_debug_default})
option(NETRX_ENABLE_CLANG_TIDY "Run clang-tidy on every compiled source" ${_netrx_debug_default})
option(NETRX_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# --- Warnings ---------------------------------------------------------------
# Applied per target via netrx_target_defaults() so third-party code
# (Catch2 via FetchContent) is not held to the same bar.
add_library(netrx_warnings INTERFACE)
if(MSVC)
  target_compile_options(netrx_warnings INTERFACE /W4 /permissive-
    $<$<BOOL:${NETRX_WARNINGS_AS_ERRORS}>:/WX>)
else()
  target_compile_options(netrx_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
    $<$<BOOL:${NETRX_WARNINGS_AS_ERRORS}>:-Werror>)
endif()

# --- Sanitizers -------------------------------------------------------------
# Global (add_compile_options / add_link_options) on purpose: every object in
# the process, including Catch2, must agree on ASan instrumentation.
if(NETRX_ENABLE_SANITIZERS)
  if(MSVC)
    # MSVC has ASan but no UBSan.
    add_compile_options(/fsanitize=address)
  else()
    set(_netrx_san_flags -fsanitize=address,undefined
                         -fno-sanitize-recover=all
                         -fno-omit-frame-pointer)
    add_compile_options(${_netrx_san_flags})
    add_link_options(-fsanitize=address,undefined)
  endif()
  message(STATUS "netrx: ASan + UBSan enabled")
endif()

# --- clang-tidy -------------------------------------------------------------
# Checks live in .clang-tidy at the repo root.
if(NETRX_ENABLE_CLANG_TIDY)
  find_program(NETRX_CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
  set(NETRX_CLANG_TIDY_COMMAND
    ${NETRX_CLANG_TIDY_EXE}
    # Same libstdc++ <format> bfloat16 workaround as .clangd.
    --extra-arg=-U__STDCPP_BFLOAT16_T__)
  message(STATUS "netrx: clang-tidy enabled (${NETRX_CLANG_TIDY_EXE})")
endif()

# Apply warnings + clang-tidy to one of our own targets.
function(netrx_target_defaults target)
  target_link_libraries(${target} PRIVATE netrx_warnings)
  if(NETRX_ENABLE_CLANG_TIDY)
    set_target_properties(${target} PROPERTIES
      CXX_CLANG_TIDY "${NETRX_CLANG_TIDY_COMMAND}")
  endif()
endfunction()
