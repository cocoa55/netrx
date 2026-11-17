#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace netrx::bench {

enum class Kind : std::uint8_t { Add, Execute, Cancel, Delete, Replace };

struct Op {
  std::uint32_t ref;    // order reference (old ref for Replace)
  std::uint32_t newRef; // only for Replace
  std::uint32_t shares; // Add / Execute / Cancel / Replace shares
  std::uint16_t locate; // stock locate
  std::uint8_t kind;    // Kind
};
static_assert(sizeof(Op) == 16);

// Walk the whole ITCH file with the netrx decoder and keep only the messages
// that touch the order table. Exits the process with a message on any decode
// error, or if a ref does not fit in 32 bits (or is 0, RefMap's empty key).
std::vector<Op> BuildTrace(const std::filesystem::path &itchFile);

} // namespace netrx::bench
