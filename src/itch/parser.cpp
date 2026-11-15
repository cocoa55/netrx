#include <array>
#include <cassert>
#include <cstdint>
#include <print>
#include <unordered_set>
#include <variant>

#include "netrx/bytes.hpp"
#include "netrx/itch/decoder.hpp"
#include "netrx/itch/parser.hpp"

#include <utility>
namespace netrx::itch {

namespace {
// Functors for each message type.
struct Checker {
  void operator()(const AddOrder &m) {
    ++counts.add;
    const bool added = live.insert(m.orderReferenceNumber).second;
    assert(added);
  }
  void operator()(const OrderDelete &m) {
    ++counts.del;
    const auto n = live.erase(m.orderReferenceNumber);
    assert(n == 1);
  }
  void operator()(const SystemEvent &m) {
    ++counts.system;
    lastEventCode = m.eventCode;
    counts.lastEventCode = m.eventCode;
  }
  void operator()(const OrderReplace &m) {
    ++counts.replace;
    const auto n = live.erase(m.originalOrderReferenceNumber);
    assert(n == 1);
    const auto added = live.insert(m.newOrderReferenceNumber).second;
    assert(added);
  }
  void operator()(const OrderCancel &m) {
    ++counts.cancel;
    assert(live.contains(m.orderReferenceNumber));
  }
  void operator()(const StockDirectory &) { ++counts.directory; }
  void operator()(const AddOrderMpid &m) {
    ++counts.addMpid;
    const auto added = live.insert(m.orderReferenceNumber).second;
    assert(added);
  }
  void operator()(const OrderExecuted &m) {
    ++counts.executed;
    assert(live.contains(m.orderReferenceNumber));
  }
  void operator()(const OrderExecutedWithPrice &m) {
    ++counts.executedPrice;
    assert(live.contains(m.orderReferenceNumber));
  }
  void operator()(const std::monostate &) { ++counts.unknown; }

  Counts counts;
  std::unordered_set<uint64_t> live; // Each message updates live
  EventCode lastEventCode{};
};
} // namespace

// Cut the file into messages
std::expected<Counts, DriverError>parseItch(std::span<const char> view) {
  // Hold information on the parsing
  Checker check;
  std::array<uint64_t, 256> unknownByType{};
  uint64_t msgIndex = 0;

  // Parse file as one C-Style array
  auto begin = view.begin();
  while (view.end() - begin >= kLengthPrefixSize) {
    auto length = netrx::readBigEndian<uint16_t>(begin);
    begin += kLengthPrefixSize;

    if (length == 0 || length > view.end() - begin) [[unlikely]] {
      std::println(stderr, "bad length {} at offset {} (message {})", length,
                   begin - view.begin(), msgIndex);
      return std::unexpected(DriverError::BadLength);
    }

    const auto v = itchDecoder({begin, length});
    if (!v.has_value()) [[unlikely]] {
      std::println(stderr,
                   "Failed to decode. itchDecoder returned {} at offset {}",
                   std::to_underlying(v.error()), begin - view.begin());
      return std::unexpected(DriverError::DecodeFailed);
    }
    if (std::holds_alternative<std::monostate>(v.value())) {
      ++unknownByType[static_cast<unsigned char>(begin[0])];
    }
    std::visit(check, v.value());
    begin += length;
    msgIndex++;
  }

  if (begin != view.end()) {
    std::println(stderr, "Trailing {} bytes at offset {}", view.end() - begin,
                 begin - view.begin());
    return std::unexpected(DriverError::TrailingBytes);
  }
  const auto &c = check.counts;
  const auto total = c.system + c.directory + c.add + c.addMpid + c.executed +
                     c.executedPrice + c.cancel + c.del + c.replace + c.unknown;

  // Print output from the parsed file
  std::println("S {} R {} A {} F {} E {} C {} X {} D {} U {} unknown {}",
               c.system, c.directory, c.add, c.addMpid, c.executed,
               c.executedPrice, c.cancel, c.del, c.replace, c.unknown);
  std::println("total {} live {} last event '{}'", total, check.live.size(),
               static_cast<char>(check.lastEventCode));
  uint64_t unknownTotal = 0;
  for (auto i{0uz}; i < unknownByType.size(); i++) {
    if (unknownByType[i] == 0)
      continue;
    std::println("'{}' (0x{:02x}) {}", static_cast<char>(i), i,
                 unknownByType[i]);
    unknownTotal += unknownByType[i];
  }
  std::println("unknown total {}", unknownTotal);
  return check.counts;
}

} // namespace netrx::itch
