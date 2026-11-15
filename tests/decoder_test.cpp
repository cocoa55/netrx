#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <expected>
#include <string>
#include <variant>
#include <vector>

#include "messages.hpp"
#include "netrx/itch/decoder.hpp"

using netrx::itch::DecodeError;
using netrx::itch::itchDecoder;

namespace {

// Names a decoder result, so a failing check prints what came back:
// "TooShort", "WrongLength", "monostate", or the type letter of the struct.
std::string describe(const std::expected<netrx::itch::Message, DecodeError> &r) {
  if (!r.has_value()) {
    if (r.error() == DecodeError::TooShort) return "TooShort";
    if (r.error() == DecodeError::WrongLength) return "WrongLength";
    return "unknown DecodeError";
  }
  const auto &m = r.value();
  if (std::holds_alternative<std::monostate>(m)) return "monostate";
  if (std::holds_alternative<netrx::itch::SystemEvent>(m)) return "S";
  if (std::holds_alternative<netrx::itch::StockDirectory>(m)) return "R";
  if (std::holds_alternative<netrx::itch::AddOrder>(m)) return "A";
  if (std::holds_alternative<netrx::itch::AddOrderMpid>(m)) return "F";
  if (std::holds_alternative<netrx::itch::OrderExecuted>(m)) return "E";
  if (std::holds_alternative<netrx::itch::OrderExecutedWithPrice>(m)) return "C";
  if (std::holds_alternative<netrx::itch::OrderCancel>(m)) return "X";
  if (std::holds_alternative<netrx::itch::OrderDelete>(m)) return "D";
  if (std::holds_alternative<netrx::itch::OrderReplace>(m)) return "U";
  return "unknown struct";
}

// Decodes a message of `type` cut or padded to exactly `length` bytes.
std::string decodeAtLength(char type, std::size_t length) {
  std::vector<char> base = test::validMessage(type);
  if (base.empty()) { // not a handled type: just the type byte
    base.push_back(type);
  }
  const std::vector<char> bytes = test::exactCopy(base, length);
  return describe(itchDecoder(bytes));
}

bool isHandled(char type) {
  for (const auto &t : test::kSpecSizes) {
    if (t.type == type) return true;
  }
  return false;
}

} // namespace

TEST_CASE("baseline: exact size decodes to the right struct", "[decoder]") {
  for (const auto &t : test::kSpecSizes) {
    CAPTURE(t.type, t.size);
    const std::vector<char> msg = test::validMessage(t.type);
    REQUIRE(msg.size() == t.size); // the builder matches the spec
    CHECK(describe(itchDecoder(msg)) == std::string(1, t.type));
  }
}

TEST_CASE("empty buffer is TooShort", "[decoder]") {
  const std::vector<char> empty;
  CHECK(describe(itchDecoder(empty)) == "TooShort");
}

TEST_CASE("header boundary: 10 bytes TooShort, 11 bytes passes", "[decoder]") {
  CHECK(decodeAtLength('H', 10) == "TooShort");
  CHECK(decodeAtLength('H', 11) == "monostate");
  for (const auto &t : test::kSpecSizes) {
    CAPTURE(t.type);
    CHECK(decodeAtLength(t.type, 10) == "TooShort");
    CHECK(decodeAtLength(t.type, 11) == "WrongLength");
  }
}

TEST_CASE("short skipped type is TooShort, not monostate", "[decoder]") {
  CHECK(decodeAtLength('H', 5) == "TooShort");
}

TEST_CASE("off by one: size - 1 and size + 1 are WrongLength", "[decoder]") {
  for (const auto &t : test::kSpecSizes) {
    CAPTURE(t.type, t.size);
    CHECK(decodeAtLength(t.type, t.size - 1) == "WrongLength");
    CHECK(decodeAtLength(t.type, t.size + 1) == "WrongLength");
  }
}

TEST_CASE("length sweep 0..64", "[decoder]") {
  for (const auto &t : test::kSpecSizes) {
    for (std::size_t length = 0; length <= 64; ++length) {
      CAPTURE(t.type, t.size, length);
      std::string expected = "WrongLength";
      if (length < 11) expected = "TooShort";
      if (length == t.size) expected = std::string(1, t.type);
      CHECK(decodeAtLength(t.type, length) == expected);
    }
  }
}

TEST_CASE("every unhandled type byte gives monostate", "[decoder]") {
  for (int b = 0; b < 256; ++b) {
    const char type = static_cast<char>(b);
    if (isHandled(type)) continue;
    for (std::size_t length : {11, 64}) {
      CAPTURE(b, length);
      CHECK(decodeAtLength(type, length) == "monostate");
    }
  }
}
