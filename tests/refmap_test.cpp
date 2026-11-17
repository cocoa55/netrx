// Tests for netrx::RefMap using only its public API
// (Insert, Find, Erase, Size, Capacity, GrowCount, end).
//
// Hash is identity, so with 8 slots a key's home is key & 7.
// Positions are checked by subtracting two iterators (Slot pointers):
// Find(b) - Find(a) == how many slots b sits after a.

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <random>
#include <unordered_map>

#include "netrx/refmap.hpp"

using Map = netrx::RefMap<std::uint64_t, int>;

// Compile every member function, even ones no test calls yet, so warnings
// like [[nodiscard]] and template errors show up.
template class netrx::RefMap<std::uint64_t, int>;

TEST_CASE("collisions probe to the next free slots", "[refmap]") {
  Map m(8);
  m.Insert(3, 100);  // home 3 -> slot 3
  m.Insert(11, 200); // home 3 -> slot 4
  m.Insert(19, 300); // home 3 -> slot 5

  CHECK(m.Size() == 3);
  REQUIRE(m.Find(3) != m.end());
  REQUIRE(m.Find(11) != m.end());
  REQUIRE(m.Find(19) != m.end());
  CHECK(m.Find(3)->second == 100);
  CHECK(m.Find(11)->second == 200);
  CHECK(m.Find(19)->second == 300);
  CHECK(m.Find(11) - m.Find(3) == 1);
  CHECK(m.Find(19) - m.Find(3) == 2);
  CHECK(m.Find(27) == m.end()); // home 3, never inserted
}

TEST_CASE("erase shifts the next entry back into the hole", "[refmap]") {
  Map m(8);
  m.Insert(3, 100);
  m.Insert(11, 200);
  m.Insert(19, 300);

  m.Erase(m.Find(11));
  CHECK(m.Size() == 2);
  CHECK(m.Find(11) == m.end());
  REQUIRE(m.Find(19) != m.end());
  CHECK(m.Find(19)->second == 300);
  CHECK(m.Find(19) - m.Find(3) == 1); // 19 moved from slot 5 to slot 4
}

TEST_CASE("erase shifts back across the wrap", "[refmap]") {
  Map m(8);
  m.Insert(6, 1);  // slot 6
  m.Insert(7, 2);  // slot 7
  m.Insert(15, 3); // home 7, taken -> wraps to slot 0

  CHECK(m.Find(6) - m.Find(15) == 6); // 15 is at slot 0

  m.Erase(m.Find(7));
  REQUIRE(m.Find(15) != m.end());
  CHECK(m.Find(15)->second == 3);
  CHECK(m.Find(15) - m.Find(6) == 1); // 15 moved back to slot 7
  CHECK(m.Size() == 2);
}

TEST_CASE("erase skips an entry at its home but moves the one behind it",
          "[refmap]") {
  Map m(8);
  m.Insert(7, 1);  // slot 7
  m.Insert(15, 2); // home 7 -> slot 0
  m.Insert(1, 3);  // home 1 -> slot 1
  m.Insert(23, 4); // home 7 -> 7, 0, 1 taken -> slot 2

  m.Erase(m.Find(7));
  // 15 moves 0 -> 7, 1 stays at its home (slot 1), 23 moves 2 -> 0.
  REQUIRE(m.Find(15) != m.end());
  REQUIRE(m.Find(1) != m.end());
  REQUIRE(m.Find(23) != m.end());
  CHECK(m.Find(15)->second == 2);
  CHECK(m.Find(1)->second == 3);
  CHECK(m.Find(23)->second == 4);
  CHECK(m.Find(15) - m.Find(1) == 6);  // 15 at slot 7
  CHECK(m.Find(1) - m.Find(23) == 1);  // 23 at slot 0
  CHECK(m.Find(7) == m.end());
  CHECK(m.Size() == 3);
}

TEST_CASE("grows and keeps every key findable", "[refmap]") {
  Map m(8);
  for (std::uint64_t k = 1; k <= 100; ++k) {
    m.Insert(k, static_cast<int>(k * 10));
  }
  CHECK(m.Size() == 100);
  CHECK(m.Capacity() == 256); // max load 1/2: 8->16->32->64->128->256
  CHECK(m.GrowCount() == 5);
  for (std::uint64_t k = 1; k <= 100; ++k) {
    auto it = m.Find(k);
    REQUIRE(it != m.end());
    CHECK(it->second == static_cast<int>(k * 10));
  }
  CHECK(m.Find(101) == m.end());

  for (std::uint64_t k = 1; k <= 100; k += 3) {
    m.Erase(m.Find(k));
  }
  for (std::uint64_t k = 1; k <= 100; ++k) {
    CHECK((m.Find(k) == m.end()) == (k % 3 == 1));
  }
  CHECK(m.Size() == 66);
}

TEST_CASE("random inserts and erases match std::unordered_map", "[refmap]") {
  for (unsigned seed = 0; seed < 20; ++seed) {
    Map m(8);
    std::unordered_map<std::uint64_t, int> ref;
    std::mt19937_64 rng(seed);

    for (int op = 0; op < 20'000; ++op) {
      const std::uint64_t key = 1 + rng() % 500;
      if (ref.contains(key)) {
        auto it = m.Find(key);
        REQUIRE(it != m.end());
        REQUIRE(it->second == ref[key]);
        m.Erase(it);
        ref.erase(key);
      } else {
        m.Insert(key, op);
        ref[key] = op;
      }
      REQUIRE(m.Size() == ref.size());
    }
    for (std::uint64_t key = 1; key <= 500; ++key) {
      auto it = m.Find(key);
      auto rit = ref.find(key);
      REQUIRE((it == m.end()) == (rit == ref.end()));
      if (rit != ref.end()) {
        REQUIRE(it->second == rit->second);
      }
    }
  }
}
