#include "netrx/itch/parser.hpp"
#include "netrx/bytes.hpp"
#include "netrx/itch/decoder.hpp"
#include "netrx/orderbook.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <print>
#include <algorithm>
#include <variant>
#include <vector>
#include <utility>
namespace netrx::itch {

namespace {
// Functors for each message type.
struct Checker {
  void operator()(const AddOrder &m) {
    const auto ref = m.orderReferenceNumber;
    assert(m.buySellIndicator == 'B' || m.buySellIndicator == 'S');

    const Order order{static_cast<Side>(m.buySellIndicator), m.price, m.shares};
    auto &book = books[m.header.stockLocate];
    book.Add(ref, order);
    ++counts.add;
    liveOrders++;
    peakOrderCount = std::max(liveOrders, peakOrderCount);
  }

  void operator()(const OrderDelete &m) {
    const auto ref = m.orderReferenceNumber;
    auto &book = books[m.header.stockLocate];
    book.Delete(ref);
    ++counts.del;

    assert(liveOrders > 0);
    liveOrders--;
    peakOrderCount = std::max(liveOrders, peakOrderCount);
  }

  void operator()(const SystemEvent &m) {
    lastEventCode = m.eventCode;
    counts.lastEventCode = m.eventCode;
    ++counts.system;
  }

  void operator()(const OrderReplace &m) {
    const auto oldRef = m.originalOrderReferenceNumber;
    const auto newRef = m.newOrderReferenceNumber;
    const auto shares = m.shares;
    const auto price = m.price;

    auto &book = books[m.header.stockLocate];
    book.Replace(oldRef, newRef, shares, price);
    ++counts.replace;
  }

  void operator()(const OrderCancel &m) {
    const auto ref = m.orderReferenceNumber;
    const auto shares = m.cancelledShares;

    auto &book = books[m.header.stockLocate];
    const size_t before = book.OrderCount();
    book.Cancel(ref, shares);
    const size_t after = book.OrderCount();
    assert(before >= after);

    const size_t removed = before - after;
    assert(liveOrders >= removed);

    liveOrders -= removed;
    ++counts.cancel;
  }

  void operator()(const StockDirectory &) { ++counts.directory; }
  void operator()(const AddOrderMpid &m) {
    const auto ref = m.orderReferenceNumber;
    assert(m.buySellIndicator == 'B' || m.buySellIndicator == 'S');

    const Order order{static_cast<Side>(m.buySellIndicator), m.price, m.shares};
    auto &book = books[m.header.stockLocate];
    book.Add(ref, order);
    ++counts.addMpid;
    liveOrders++;
    peakOrderCount = std::max(liveOrders, peakOrderCount);
  }

  void operator()(const OrderExecuted &m) {
    const auto ref = m.orderReferenceNumber;
    const auto shares = m.executedShares;

    auto &book = books[m.header.stockLocate];

    const size_t before = book.OrderCount();
    book.Execute(ref, shares);
    const size_t after = book.OrderCount();
    assert(before >= after);

    const size_t removed = before - after;
    assert(liveOrders >= removed);

    liveOrders -= removed;
    ++counts.executed;
  }

  void operator()(const OrderExecutedWithPrice &m) {
    const auto ref = m.orderReferenceNumber;
    const auto shares = m.executedShares;

    auto &book = books[m.header.stockLocate];

    const size_t before = book.OrderCount();
    book.Execute(ref, shares);
    const size_t after = book.OrderCount();
    assert(before >= after);

    const size_t removed = before - after;
    assert(liveOrders >= removed);

    liveOrders -= removed;
    ++counts.executedPrice;
  }

  void operator()(const std::monostate &) { ++counts.unknown; }

  Counts counts;
  std::vector<Orderbook> books = std::vector<Orderbook>(65'536);
  size_t liveOrders{};
  size_t peakOrderCount{};
  EventCode lastEventCode{};
};
} // namespace

// Cut the file into messages
std::expected<Counts, DriverError> parseItch(std::span<const char> view) {
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
  std::println("total {} live {} peak {} last event '{}'", total, check.liveOrders,check.peakOrderCount,
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
