#include <cstdint>
#include <print>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

#include "netrx/itch/decoder.hpp"
#include "netrx/itch/messages.hpp"

namespace {
int failures = 0;

void check(bool ok, std::string_view what) {
  std::println("{} {}", ok ? "PASS" : "FAIL", what);
  if (!ok) {
    ++failures;
  }
}

template <typename T> void putBigEndian(std::vector<char> &out, T value) {
  for (int shift = (sizeof(T) - 1) * 8; shift >= 0; shift -= 8) {
    out.push_back(static_cast<char>((value >> shift) & 0xFF));
  }
}

std::string_view stockView(const netrx::itch::Stock &stock) {
  return {stock.data(), stock.size()};
}

constexpr std::string_view kExpectedStock = "AAPL    ";
constexpr uint64_t kExpectedOrderRef = 0x0102030405060708;
constexpr uint32_t kExpectedShares = 100;
constexpr uint32_t kExpectedPrice = 1'500'000; // $150.0000 (4 implied decimals)

// Body only: everything after the 11-byte header (offset 11 onward).
std::vector<char> addOrderBody() {
  std::vector<char> body;
  putBigEndian<uint64_t>(body, kExpectedOrderRef);
  body.push_back('B');
  putBigEndian<uint32_t>(body, kExpectedShares);
  body.insert(body.end(), kExpectedStock.begin(), kExpectedStock.end());
  putBigEndian<uint32_t>(body, kExpectedPrice);
  return body;
}

// Full message: header + body, as it appears in the file after the length.
std::vector<char> addOrderMessage() {
  std::vector<char> msg;
  msg.push_back('A');
  putBigEndian<uint16_t>(msg, 13);   // stock locate
  putBigEndian<uint16_t>(msg, 0);    // tracking number
  putBigEndian<uint16_t>(msg, 0x0000); // timestamp, high 2 bytes
  putBigEndian<uint32_t>(msg, 0x12345678); // timestamp, low 4 bytes
  const auto body = addOrderBody();
  msg.insert(msg.end(), body.begin(), body.end());
  return msg;
}

void testAddOrderDirect() {
  const auto body = addOrderBody();
  const netrx::itch::MessageHeader header{
      .messageType = netrx::itch::MessageType::AddOrder,
      .stockLocate = 13,
      .trackingNumber = 0,
      .timestamp = 0x12345678};

  const auto order = netrx::itch::addOrder(header, body);

  check(stockView(order.stock) == kExpectedStock,
        "addOrder: stock == \"AAPL    \"");
  check(order.orderReferenceNumber == kExpectedOrderRef,
        "addOrder: order reference number");
  check(order.buySellIndicator == 'B', "addOrder: buy/sell indicator");
  check(order.shares == kExpectedShares, "addOrder: shares");
  check(order.price == kExpectedPrice, "addOrder: price");
}

void testAddOrderThroughDecoder() {
  const auto msg = addOrderMessage();
  const auto decoded = netrx::itch::itchDecoder(msg);

  const auto *order = std::get_if<netrx::itch::AddOrder>(&decoded);
  check(order != nullptr, "itchDecoder: returns an AddOrder");
  if (order == nullptr) {
    return;
  }
  check(stockView(order->stock) == kExpectedStock,
        "itchDecoder: stock == \"AAPL    \"");
  check(order->header.stockLocate == 13, "itchDecoder: stock locate");
  check(order->price == kExpectedPrice, "itchDecoder: price");
}
} // namespace

int main() {
  testAddOrderDirect();
  testAddOrderThroughDecoder();

  std::println("{} failure(s)", failures);
  return failures == 0 ? 0 : 1;
}
