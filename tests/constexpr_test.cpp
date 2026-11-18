// Compile-time tests for SeqTracker and RefMap.
//
// Built twice (see tests/CMakeLists.txt):
//   netrx_constexpr_tests          STATIC_REQUIRE checks at compile time
//   netrx_relaxed_constexpr_tests  same checks at runtime, for debugging
//
// Each check is a lambda run inside STATIC_REQUIRE, so the whole body has to
// be constant-evaluable.

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <limits>
#include <utility>

#include "netrx/refmap.hpp"
#include "netrx/seqtracker.hpp"

namespace {

using netrx::SeqTracker;
using Map = netrx::RefMap<std::uint64_t, int>;

constexpr auto kMax = std::numeric_limits<std::uint64_t>::max();

} // namespace

// --- SeqTracker -------------------------------------------------------------

TEST_CASE("seqtracker: fresh tracker counts nothing", "[constexpr][seq]") {
  STATIC_REQUIRE([] {
    const SeqTracker t;
    return t.dropped() == 0 && t.reordered() == 0;
  }());
}

TEST_CASE("seqtracker: in-order packets count nothing", "[constexpr][seq]") {
  STATIC_REQUIRE([] {
    SeqTracker t;
    for (std::uint64_t s = 0; s < 100; ++s)
      t.onPacket(s);
    return t.dropped() == 0 && t.reordered() == 0;
  }());
}

TEST_CASE("seqtracker: first packet above 0 counts the skipped ones",
          "[constexpr][seq]") {
  STATIC_REQUIRE([] {
    SeqTracker t;
    t.onPacket(5);
    return t.dropped() == 5 && t.reordered() == 0;
  }());
}

TEST_CASE("seqtracker: gap counts each missing packet", "[constexpr][seq]") {
  STATIC_REQUIRE([] {
    SeqTracker t;
    t.onPacket(0);
    t.onPacket(3);
    t.onPacket(4);
    return t.dropped() == 2 && t.reordered() == 0;
  }());
}

TEST_CASE("seqtracker: late packet undoes one drop", "[constexpr][seq]") {
  STATIC_REQUIRE([] {
    SeqTracker t;
    t.onPacket(0);
    t.onPacket(2);
    t.onPacket(1);
    return t.dropped() == 0 && t.reordered() == 1;
  }());
}

TEST_CASE("seqtracker: duplicate counts as reordered", "[constexpr][seq]") {
  STATIC_REQUIRE([] {
    SeqTracker t;
    t.onPacket(0);
    t.onPacket(0);
    return t.dropped() == 0 && t.reordered() == 1;
  }());
}

// Current behavior: a duplicate after a gap is treated like the missing
// packet arriving late, so the real drop of seq 1 is no longer counted.
TEST_CASE("seqtracker: duplicate after a gap cancels the drop",
          "[constexpr][seq]") {
  STATIC_REQUIRE([] {
    SeqTracker t;
    t.onPacket(0);
    t.onPacket(2);
    t.onPacket(2);
    return t.dropped() == 0 && t.reordered() == 1;
  }());
}

// Current behavior: expected_seq_ wraps to 0 after UINT64_MAX, so 0 is then
// in order.
TEST_CASE("seqtracker: UINT64_MAX wraps the expected seq to 0",
          "[constexpr][seq]") {
  STATIC_REQUIRE([] {
    SeqTracker t;
    t.onPacket(kMax);
    t.onPacket(0);
    return t.dropped() == kMax && t.reordered() == 0;
  }());
}

// --- RefMap -----------------------------------------------------------------
// Hash is identity, so with 8 slots a key's home is key & 7.

TEST_CASE("refmap: empty map finds nothing", "[constexpr][refmap]") {
  STATIC_REQUIRE([] {
    const Map m(8);
    return m.Size() == 0 && m.Capacity() == 8 && m.GrowCount() == 0 &&
           m.Find(1) == m.end();
  }());
}

TEST_CASE("refmap: one insert is findable through const and non-const",
          "[constexpr][refmap]") {
  STATIC_REQUIRE([] {
    Map m(8);
    m.Insert(3, 100);
    const Map &cm = m;
    return m.Size() == 1 && m.Find(3) != m.end() && m.Find(3)->second == 100 &&
           cm.Find(3) != cm.end() && cm.Find(3)->second == 100;
  }());
}

TEST_CASE("refmap: capacity 1 grows on the first insert",
          "[constexpr][refmap]") {
  STATIC_REQUIRE([] {
    Map m(1);
    m.Insert(1, 10);
    return m.Capacity() == 2 && m.GrowCount() == 1 && m.Find(1)->second == 10;
  }());
}

TEST_CASE("refmap: collisions probe to the next free slots",
          "[constexpr][refmap]") {
  STATIC_REQUIRE([] {
    Map m(8);
    m.Insert(3, 100);
    m.Insert(11, 200);
    m.Insert(19, 300);
    return m.Find(11) - m.Find(3) == 1 && m.Find(19) - m.Find(3) == 2 &&
           m.Find(27) == m.end();
  }());
}

TEST_CASE("refmap: erase shifts the next entry back into the hole",
          "[constexpr][refmap]") {
  STATIC_REQUIRE([] {
    Map m(8);
    m.Insert(3, 100);
    m.Insert(11, 200);
    m.Insert(19, 300);
    m.Erase(m.Find(11));
    return m.Size() == 2 && m.Find(11) == m.end() &&
           m.Find(19)->second == 300 && m.Find(19) - m.Find(3) == 1;
  }());
}

TEST_CASE("refmap: erase shifts back across the wrap", "[constexpr][refmap]") {
  STATIC_REQUIRE([] {
    Map m(8);
    m.Insert(6, 1);
    m.Insert(7, 2);
    m.Insert(15, 3); // home 7 -> wraps to slot 0
    m.Erase(m.Find(7));
    return m.Find(15) - m.Find(6) == 1 && m.Find(15)->second == 3;
  }());
}

TEST_CASE("refmap: erasing the only entry empties the map",
          "[constexpr][refmap]") {
  STATIC_REQUIRE([] {
    Map m(8);
    m.Insert(5, 1);
    m.Erase(m.Find(5));
    return m.Size() == 0 && m.Find(5) == m.end();
  }());
}

TEST_CASE("refmap: grows only past the 1/2 load limit", "[constexpr][refmap]") {
  STATIC_REQUIRE([] {
    Map m(8);
    for (std::uint64_t k = 1; k <= 4; ++k)
      m.Insert(k, static_cast<int>(k));
    const bool atLimit = m.Capacity() == 8 && m.GrowCount() == 0;
    m.Insert(5, 5);
    const bool grown = m.Capacity() == 16 && m.GrowCount() == 1;
    bool allFound = true;
    for (std::uint64_t k = 1; k <= 5; ++k)
      allFound = allFound && m.Find(k) != m.end() &&
                 std::cmp_equal(m.Find(k)->second, k);
    return atLimit && grown && allFound;
  }());
}

TEST_CASE("refmap: max key lands in the last slot", "[constexpr][refmap]") {
  STATIC_REQUIRE([] {
    Map m(8);
    m.Insert(kMax, 7);
    m.Insert(8, 0); // home 0
    return m.Find(kMax) - m.Find(8) == 7 && m.Find(kMax)->second == 7;
  }());
}
