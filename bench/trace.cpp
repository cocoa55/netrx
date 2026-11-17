#include "trace.hpp"

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <print>
#include <utility>
#include <variant>

#include "netrx/bytes.hpp"
#include "netrx/itch/decoder.hpp"
#include "netrx/mappedfile.hpp"

namespace netrx::bench {
namespace {

[[noreturn]] void Fail(const char *what, std::uint64_t msgIndex) {
  std::println(stderr, "trace build failed: {} (message {})", what, msgIndex);
  std::exit(1);
}

// Checked in every build type (assert() is compiled out under NDEBUG).
std::uint32_t Ref32(std::uint64_t ref, std::uint64_t msgIndex) {
  if (ref > std::numeric_limits<std::uint32_t>::max())
    Fail("order ref >= 2^32", msgIndex);
  if (ref == 0)
    Fail("order ref == 0 (RefMap's empty key)", msgIndex);
  return static_cast<std::uint32_t>(ref);
}

struct Collector {
  std::vector<Op> &out;
  std::uint64_t msgIndex{};

  void Push(Kind k, std::uint64_t ref, std::uint64_t newRef,
            std::uint32_t shares, std::uint16_t locate) {
    out.push_back(Op{.ref = Ref32(ref, msgIndex),
                     .newRef = k == Kind::Replace ? Ref32(newRef, msgIndex) : 0,
                     .shares = shares,
                     .locate = locate,
                     .kind = std::to_underlying(k)});
  }

  void operator()(const itch::AddOrder &m) {
    Push(Kind::Add, m.orderReferenceNumber, 0, m.shares, m.header.stockLocate);
  }
  void operator()(const itch::AddOrderMpid &m) {
    Push(Kind::Add, m.orderReferenceNumber, 0, m.shares, m.header.stockLocate);
  }
  void operator()(const itch::OrderExecuted &m) {
    Push(Kind::Execute, m.orderReferenceNumber, 0, m.executedShares,
         m.header.stockLocate);
  }
  void operator()(const itch::OrderExecutedWithPrice &m) {
    Push(Kind::Execute, m.orderReferenceNumber, 0, m.executedShares,
         m.header.stockLocate);
  }
  void operator()(const itch::OrderCancel &m) {
    Push(Kind::Cancel, m.orderReferenceNumber, 0, m.cancelledShares,
         m.header.stockLocate);
  }
  void operator()(const itch::OrderDelete &m) {
    Push(Kind::Delete, m.orderReferenceNumber, 0, 0, m.header.stockLocate);
  }
  void operator()(const itch::OrderReplace &m) {
    Push(Kind::Replace, m.originalOrderReferenceNumber,
         m.newOrderReferenceNumber, m.shares, m.header.stockLocate);
  }
  // S, R and unknown types don't touch the order table.
  void operator()(const auto &) {}
};

} // namespace

std::vector<Op> BuildTrace(const std::filesystem::path &itchFile) {
  const MappedFile file{itchFile};
  const auto view = file.data();
  if (view.empty())
    Fail("could not map the ITCH file (or it is empty)", 0);

  std::vector<Op> trace;
  trace.reserve(view.size() / 32); // rough upper bound, avoids most regrowth
  Collector collect{trace};

  // Same framing loop as src/itch/parser.cpp: 2-byte big-endian length, then
  // one message, decoded by itch::itchDecoder.
  auto begin = view.begin();
  while (view.end() - begin >= kLengthPrefixSize) {
    const auto length = readBigEndian<std::uint16_t>(begin);
    begin += kLengthPrefixSize;
    if (length == 0 || length > view.end() - begin)
      Fail("bad length prefix", collect.msgIndex);

    const auto msg = itch::itchDecoder({begin, length});
    if (!msg.has_value())
      Fail("itchDecoder returned an error", collect.msgIndex);
    std::visit(collect, *msg);

    begin += length;
    ++collect.msgIndex;
  }
  if (begin != view.end())
    Fail("trailing bytes at end of file", collect.msgIndex);

  trace.shrink_to_fit();
  return trace;
}

} // namespace netrx::bench
