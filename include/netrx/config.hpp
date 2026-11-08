#pragma once

#include <cstdint>

#include <chrono>
#include <cstddef>

namespace netrx {
inline constexpr size_t ITERATIONS{1000000};
inline constexpr std::chrono::microseconds INTERVAL{10};
inline constexpr std::uint16_t PORT{9000};
inline constexpr std::chrono::seconds RECV_TIMEOUT{1};
inline constexpr std::chrono::microseconds SLEEP_MARGIN{2};
} // namespace netrx
