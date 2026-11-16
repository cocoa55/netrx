#pragma once
#include "netrx/itch/messages.hpp"

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
  Order(const Side side, const Price price, const Shares shares)
      : price_{price}, shares_{shares}, side_{side} {}

  [[nodiscard]] bool isFilled() const { return shares_ == 0; }

  [[nodiscard]] auto price() const { return price_; }

  [[nodiscard]] auto quantity() const { return shares_; }

  [[nodiscard]] auto side() const { return side_; }

  void fillOrder(const Shares quantity) {
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
  Orderbook &operator=(const Orderbook &) = delete;
  Orderbook(Orderbook &&) = default;
  Orderbook &operator=(Orderbook &&) = default;
  ~Orderbook() = default;

  void Add(itch::OrderReferenceNumber ref, Order order);

  void Execute(itch::OrderReferenceNumber ref, Shares executedShares);

  void Cancel(itch::OrderReferenceNumber ref, Shares cancelledShares);

  void Delete(itch::OrderReferenceNumber ref);

  void Replace(itch::OrderReferenceNumber oldRef,
               itch::OrderReferenceNumber newRef, Shares shares, Price price);

  auto BestBid() const -> std::optional<Price>;

  auto BestAsk() const -> std::optional<Price>;

  [[nodiscard]] auto OrderCount() const -> size_t { return orders_.size(); }


private:
  auto RemoveOrder(const itch::OrderReferenceNumber ref) { orders_.erase(ref); }
  auto &GetSide(const Side side) { return side == Side::Ask ? asks_ : bids_; }

  using OrderIt =
      std::unordered_map<itch::OrderReferenceNumber, Order>::iterator;
  void ReduceOrder(OrderIt orderIt, Shares shares);

  std::unordered_map<itch::OrderReferenceNumber, Order> orders_;
  // Levels
  std::flat_map<Price, Shares> bids_;
  std::flat_map<Price, Shares> asks_;
};
static_assert(std::is_move_constructible_v<Orderbook>);
static_assert(!std::is_copy_constructible_v<Orderbook>);
} // namespace netrx