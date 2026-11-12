#pragma once

#include <bit>
#include <concepts>
#include <cstring>
#include <memory>
#include <span>

namespace netrx {
// Every ITCH message in the file is preceded by a 2-byte big-endian length.
inline constexpr auto kLengthPrefixSize = 2;

// Templates stay header-only so every translation unit can instantiate them.
template <typename T>
  requires std::unsigned_integral<T>
T readBigEndian(const std::span<const char>::iterator begin) {
  T value{};
  std::memcpy(&value, std::to_address(begin), sizeof(T));

  return std::byteswap(value);
}

template <typename T>
  requires std::unsigned_integral<T>
T readBigEndian(const std::span<const char> bytes) {
  T value{};
  std::memcpy(&value, std::to_address(bytes.data()), sizeof(T));

  return std::byteswap(value);
}
} // namespace netrx
