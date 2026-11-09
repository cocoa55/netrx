#pragma once

#include <cstdint>

#include <chrono>
#include <cstddef>

namespace netrx {
inline constexpr size_t kIterations{1000000};
inline constexpr std::chrono::microseconds kInterval{10};
inline constexpr std::uint16_t kPort{9000};
inline constexpr std::chrono::seconds kRecvTimeout{1};
} // namespace netrx
