#pragma once

#include <cstdint>
#include <expected>
#include <span>

#include "netrx/itch/messages.hpp"

namespace netrx::itch {
struct Counts {
  uint64_t system = 0, directory = 0, add = 0, addMpid = 0, executed = 0,
           executedPrice = 0, cancel = 0, del = 0, replace = 0, unknown = 0;
  EventCode lastEventCode{};
};

enum class DriverError { BadLength, DecodeFailed, TrailingBytes };

// Cut the file into messages
std::expected<Counts, DriverError> parseItch(std::span<const char> view);
} // namespace netrx::itch
