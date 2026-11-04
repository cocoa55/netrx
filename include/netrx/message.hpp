#pragma once
#include <cstdint>

#include <bit>
#include <type_traits>

namespace netrx {
enum class Side : uint8_t { Ask, Bid };
enum class MsgType : uint8_t { Add, Cancel };
struct Message {
  uint64_t send_time_ns;
  uint64_t seq_id;
  uint64_t order_id;
  uint64_t price;
  uint32_t quantity;
  uint16_t instrument_id;
  Side side;
  MsgType type;
};
static_assert(sizeof(Message) == 40);
static_assert(std::is_trivially_copyable_v<Message>);
static_assert(std::endian::native == std::endian::little);
} // namespace netrx
