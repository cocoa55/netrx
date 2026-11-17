#pragma once
#include "netrx/itch/messages.hpp"
#include "netrx/refmap.hpp"
#include <cassert>
#include <flat_map>
#include <optional>
#include <unordered_map>

using Shares = uint32_t;
using Price = uint32_t;

namespace netrx {
enum class Side : char { Bid = 'B', Ask = 'S' };
struct LevelInfo {
  Price price;
  Shares quantity;
};

class Order {
public:
  constexpr Order() = default;
  constexpr Order(const Side side, const Price price,
                  const Shares shares) noexcept
      : price_{price}, shares_{shares}, side_{side} {}

  [[nodiscard]] constexpr auto isFilled() const noexcept -> bool {
    return shares_ == 0;
  }

  [[nodiscard]] constexpr auto price() const noexcept -> Price {
    return price_;
  }

  [[nodiscard]] constexpr auto quantity() const noexcept -> Shares {
    return shares_;
  }

  [[nodiscard]] constexpr auto side() const noexcept -> Side { return side_; }

  constexpr auto fillOrder(const Shares quantity) noexcept -> void {
    assert(quantity <= shares_);
    shares_ -= quantity;
  }

private:
  Price price_;
  Shares shares_;
  Side side_;
};
static_assert(sizeof(Order) == 12);

class Orderbook {
public:
  Orderbook() = default;
  Orderbook(const Orderbook &) = delete;
  auto operator=(const Orderbook &) -> Orderbook & = delete;
  Orderbook(Orderbook &&) = default;
  auto operator=(Orderbook &&) -> Orderbook & = default;
  ~Orderbook() = default;

  auto Add(itch::OrderReferenceNumber ref, Order order) -> void;

  auto Execute(itch::OrderReferenceNumber ref, Shares executedShares) noexcept
      -> void;

  auto Cancel(itch::OrderReferenceNumber ref, Shares cancelledShares) noexcept
      -> void;

  auto Delete(itch::OrderReferenceNumber ref) noexcept -> void;

  auto Replace(itch::OrderReferenceNumber oldRef,
               itch::OrderReferenceNumber newRef, Shares shares, Price price)
      -> void;

  [[nodiscard]] auto BestBid() const noexcept -> std::optional<Price>;

  [[nodiscard]] auto BestAsk() const noexcept -> std::optional<Price>;

  [[nodiscard]] auto OrderCount() const noexcept -> size_t {
    return orders_.Size();
  }

private:
  using Levels = std::flat_map<Price, Shares>;
  using OrderIt = RefMap<itch::OrderReferenceNumber, Order>::iterator;

  auto GetSide(const Side side) noexcept -> Levels & {
    return side == Side::Ask ? asks_ : bids_;
  }

  auto ReduceOrder(OrderIt orderIt, Shares shares) noexcept -> void;

  RefMap<itch::OrderReferenceNumber, Order> orders_{16};
  Levels bids_;
  Levels asks_;
};
static_assert(std::is_move_constructible_v<Orderbook>);
static_assert(!std::is_copy_constructible_v<Orderbook>);
} // namespace netrx