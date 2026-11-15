#include <catch2/catch_test_macros.hpp>

#include <expected>
#include <string>
#include <vector>

#include "messages.hpp"
#include "netrx/itch/parser.hpp"

using netrx::itch::DriverError;
using netrx::itch::parseItch;

namespace {

// Names a driver result, so a failing check prints what came back:
// "BadLength", "DecodeFailed", "TrailingBytes", or "ok".
std::string describe(
    const std::expected<netrx::itch::Counts, DriverError> &r) {
  if (r.has_value()) return "ok";
  if (r.error() == DriverError::BadLength) return "BadLength";
  if (r.error() == DriverError::DecodeFailed) return "DecodeFailed";
  if (r.error() == DriverError::TrailingBytes) return "TrailingBytes";
  return "unknown DriverError";
}

// Copies a built stream into a vector of exactly its size.
std::vector<char> exact(const std::vector<char> &stream) {
  return test::exactCopy(stream, stream.size());
}

} // namespace

TEST_CASE("two valid messages back to back", "[driver]") {
  std::vector<char> stream;
  test::putFramed(stream, 12, test::systemEvent('O'));
  test::putFramed(stream, 36, test::addOrder(1));

  const auto r = parseItch(exact(stream));
  REQUIRE(describe(r) == "ok");
  CHECK(r->system == 1);
  CHECK(r->add == 1);
  CHECK(r->directory == 0);
  CHECK(r->addMpid == 0);
  CHECK(r->executed == 0);
  CHECK(r->executedPrice == 0);
  CHECK(r->cancel == 0);
  CHECK(r->del == 0);
  CHECK(r->replace == 0);
  CHECK(r->unknown == 0);
}

TEST_CASE("length prefix of 0 is BadLength", "[driver]") {
  std::vector<char> stream;
  test::putFramed(stream, 0, test::systemEvent('O'));
  CHECK(describe(parseItch(exact(stream))) == "BadLength");
}

TEST_CASE("length prefix one bigger than bytes left is BadLength",
          "[driver]") {
  std::vector<char> stream;
  test::putFramed(stream, 13, test::systemEvent('O')); // 12 bytes follow
  CHECK(describe(parseItch(exact(stream))) == "BadLength");
}

TEST_CASE("length prefix exactly equal to bytes left succeeds", "[driver]") {
  std::vector<char> stream;
  test::putFramed(stream, 12, test::systemEvent('O')); // 12 bytes follow
  const auto r = parseItch(exact(stream));
  REQUIRE(describe(r) == "ok");
  CHECK(r->system == 1);
}

TEST_CASE("one leftover byte after a valid message is TrailingBytes",
          "[driver]") {
  std::vector<char> stream;
  test::putFramed(stream, 12, test::systemEvent('O'));
  stream.push_back(0x00); // half a length prefix
  CHECK(describe(parseItch(exact(stream))) == "TrailingBytes");
}

TEST_CASE("prefix of 10 with 10 bytes after it is DecodeFailed", "[driver]") {
  std::vector<char> stream;
  test::putFramed(stream, 10, test::exactCopy(test::systemEvent('O'), 10));
  CHECK(describe(parseItch(exact(stream))) == "DecodeFailed");
}

TEST_CASE("A framed as 35 bytes is DecodeFailed", "[driver]") {
  std::vector<char> stream;
  test::putFramed(stream, 35, test::exactCopy(test::addOrder(1), 35));
  CHECK(describe(parseItch(exact(stream))) == "DecodeFailed");
}

TEST_CASE("empty stream succeeds with all counts at zero", "[driver]") {
  const std::vector<char> stream;
  const auto r = parseItch(stream);
  REQUIRE(describe(r) == "ok");
  CHECK(r->system == 0);
  CHECK(r->directory == 0);
  CHECK(r->add == 0);
  CHECK(r->addMpid == 0);
  CHECK(r->executed == 0);
  CHECK(r->executedPrice == 0);
  CHECK(r->cancel == 0);
  CHECK(r->del == 0);
  CHECK(r->replace == 0);
  CHECK(r->unknown == 0);
}
