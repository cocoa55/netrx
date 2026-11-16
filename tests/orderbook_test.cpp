// Tests for netrx::Orderbook using only its public API (Add, Execute,
// Cancel, Delete, Replace, BestBid, BestAsk).
//
// Prices are ITCH prices: integers with 4 implied decimals, so 10.01 is
// 100100. Every check is an exact value; std::nullopt means "side empty".

#include <catch2/catch_test_macros.hpp>

#include <optional>

#include "netrx/orderbook.hpp"

using netrx::Order;
using netrx::Orderbook;
using netrx::Side;

namespace {

// Dollars and cents to an ITCH price: px(10, 1) == 100100 (10.01).
constexpr Price px(unsigned dollars, unsigned cents) {
  return dollars * 10000 + cents * 100;
}

constexpr std::optional<Price> none = std::nullopt;

} // namespace

TEST_CASE("empty book has no best bid or ask", "[book]") {
  const Orderbook book;
  CHECK(book.BestBid() == none);
  CHECK(book.BestAsk() == none);
}

// The 9 step worked example from the design thread. Best bid and best ask
// are checked after every step, not only at the end.
TEST_CASE("nine step worked example", "[book]") {
  Orderbook book;

  // 1. A: ref 111, Buy 200 @ 10.01
  book.Add(111, Order{Side::Bid, px(10, 1), 200});
  CHECK(book.BestBid() == px(10, 1));
  CHECK(book.BestAsk() == none);

  // 2. A: ref 222, Buy 500 @ 10.01
  book.Add(222, Order{Side::Bid, px(10, 1), 500});
  CHECK(book.BestBid() == px(10, 1));
  CHECK(book.BestAsk() == none);

  // 3. F: ref 333, Buy 100 @ 10.00 (MPID ignored)
  book.Add(333, Order{Side::Bid, px(10, 0), 100});
  CHECK(book.BestBid() == px(10, 1));
  CHECK(book.BestAsk() == none);

  // 4. A: ref 444, Sell 300 @ 10.04
  book.Add(444, Order{Side::Ask, px(10, 4), 300});
  CHECK(book.BestBid() == px(10, 1));
  CHECK(book.BestAsk() == px(10, 4));

  // 5. E: ref 222 executed 100 (222 has 400 left)
  book.Execute(222, 100);
  CHECK(book.BestBid() == px(10, 1));
  CHECK(book.BestAsk() == px(10, 4));

  // 6. C: ref 444 executed 300 at 10.03. The order and its own level
  //    (10.04) both reach 0 and are removed, so the ask side is empty.
  book.Execute(444, 300);
  CHECK(book.BestBid() == px(10, 1));
  CHECK(book.BestAsk() == none);

  // 7. X: ref 111 cancelled 50 (111 has 150 left)
  book.Cancel(111, 50);
  CHECK(book.BestBid() == px(10, 1));
  CHECK(book.BestAsk() == none);

  // 8. D: ref 333 (level 10.00 is removed)
  book.Delete(333);
  CHECK(book.BestBid() == px(10, 1));
  CHECK(book.BestAsk() == none);

  // 9. U: old 222 -> new 555, 300 @ 10.02, side taken from 222 (Buy)
  book.Replace(222, 555, 300, px(10, 2));
  CHECK(book.BestBid() == px(10, 2));
  CHECK(book.BestAsk() == none);
}

// --- Level removal --------------------------------------------------------
// If an emptied level were left in the side with 0 shares, the best price
// would not move. Each case below empties the best level and checks that
// the next level becomes best.

TEST_CASE("delete of last order at best bid removes the level", "[book]") {
  Orderbook book;
  book.Add(1, Order{Side::Bid, px(10, 0), 100});
  book.Add(2, Order{Side::Bid, px(10, 1), 100});
  REQUIRE(book.BestBid() == px(10, 1));

  book.Delete(2);
  CHECK(book.BestBid() == px(10, 0));

  book.Delete(1);
  CHECK(book.BestBid() == none);
}

TEST_CASE("delete of last order at best ask removes the level", "[book]") {
  Orderbook book;
  book.Add(1, Order{Side::Ask, px(10, 5), 100});
  book.Add(2, Order{Side::Ask, px(10, 4), 100});
  REQUIRE(book.BestAsk() == px(10, 4));

  book.Delete(2);
  CHECK(book.BestAsk() == px(10, 5));

  book.Delete(1);
  CHECK(book.BestAsk() == none);
}

TEST_CASE("partial execute keeps the level, full execute removes it",
          "[book]") {
  Orderbook book;
  book.Add(1, Order{Side::Bid, px(10, 0), 100});
  book.Add(2, Order{Side::Bid, px(10, 1), 100});

  book.Execute(2, 60);
  CHECK(book.BestBid() == px(10, 1));

  book.Execute(2, 40);
  CHECK(book.BestBid() == px(10, 0));
}

TEST_CASE("cancel of all remaining shares removes order and level",
          "[book]") {
  Orderbook book;
  book.Add(1, Order{Side::Ask, px(10, 5), 100});
  book.Add(2, Order{Side::Ask, px(10, 4), 100});

  book.Cancel(2, 30);
  CHECK(book.BestAsk() == px(10, 4));

  book.Cancel(2, 70);
  CHECK(book.BestAsk() == px(10, 5));
}

TEST_CASE("level stays while another order still rests at that price",
          "[book]") {
  Orderbook book;
  book.Add(1, Order{Side::Bid, px(10, 0), 100});
  book.Add(2, Order{Side::Bid, px(10, 1), 100});
  book.Add(3, Order{Side::Bid, px(10, 1), 50});

  book.Delete(2);
  CHECK(book.BestBid() == px(10, 1)); // order 3 still there

  book.Execute(3, 50);
  CHECK(book.BestBid() == px(10, 0));
}

// --- Ordering ------------------------------------------------------------

TEST_CASE("best bid is the highest price, best ask the lowest", "[book]") {
  Orderbook book;
  // Added out of order on purpose.
  book.Add(1, Order{Side::Bid, px(9, 98), 100});
  book.Add(2, Order{Side::Bid, px(10, 0), 100});
  book.Add(3, Order{Side::Bid, px(9, 99), 100});
  book.Add(4, Order{Side::Ask, px(10, 6), 100});
  book.Add(5, Order{Side::Ask, px(10, 4), 100});
  book.Add(6, Order{Side::Ask, px(10, 5), 100});

  CHECK(book.BestBid() == px(10, 0));
  CHECK(book.BestAsk() == px(10, 4));
}

TEST_CASE("bids and asks do not affect each other", "[book]") {
  Orderbook book;
  book.Add(1, Order{Side::Bid, px(10, 0), 100});
  CHECK(book.BestAsk() == none);

  book.Add(2, Order{Side::Ask, px(10, 2), 100});
  book.Delete(2);
  CHECK(book.BestBid() == px(10, 0));
  CHECK(book.BestAsk() == none);
}

// --- Replace -------------------------------------------------------------

TEST_CASE("replace keeps the old order's side", "[book]") {
  Orderbook book;
  book.Add(1, Order{Side::Ask, px(10, 4), 100});

  book.Replace(1, 2, 100, px(10, 3));
  CHECK(book.BestAsk() == px(10, 3));
  CHECK(book.BestBid() == none);
}

TEST_CASE("replace to a worse price moves the best bid down", "[book]") {
  Orderbook book;
  book.Add(1, Order{Side::Bid, px(10, 0), 100});
  book.Add(2, Order{Side::Bid, px(10, 1), 100});

  book.Replace(2, 3, 100, px(9, 99));
  CHECK(book.BestBid() == px(10, 0));
}

TEST_CASE("replace at the same price keeps the level", "[book]") {
  Orderbook book;
  book.Add(1, Order{Side::Bid, px(10, 1), 100});

  book.Replace(1, 2, 40, px(10, 1));
  CHECK(book.BestBid() == px(10, 1));

  // The new ref holds exactly 40 shares: executing 40 empties the side.
  book.Execute(2, 40);
  CHECK(book.BestBid() == none);
}

TEST_CASE("replace uses price and shares in the right places", "[book]") {
  // Price and Shares are both uint32_t, so a swap compiles silently.
  // Shares 300 at price 10.02: if swapped, the best bid would read 300.
  Orderbook book;
  book.Add(1, Order{Side::Bid, px(10, 1), 500});

  book.Replace(1, 2, 300, px(10, 2));
  CHECK(book.BestBid() == px(10, 2));

  book.Execute(2, 300);
  CHECK(book.BestBid() == none);
}

TEST_CASE("old ref is gone after replace, new ref is live", "[book]") {
  Orderbook book;
  book.Add(1, Order{Side::Bid, px(10, 1), 100});
  book.Replace(1, 2, 100, px(10, 1));

  // Re-adding ref 1 must succeed (it would assert if 1 were still live).
  book.Add(1, Order{Side::Bid, px(10, 0), 100});
  book.Delete(2);
  CHECK(book.BestBid() == px(10, 0));
}

// --- Refs are reusable after removal --------------------------------------

TEST_CASE("an executed-out order is removed from the order table", "[book]") {
  Orderbook book;
  book.Add(7, Order{Side::Bid, px(10, 0), 100});
  book.Execute(7, 100);
  CHECK(book.BestBid() == none);

  // Ref 7 was erased, so adding it again must not trip the duplicate assert.
  book.Add(7, Order{Side::Bid, px(10, 1), 100});
  CHECK(book.BestBid() == px(10, 1));
}
