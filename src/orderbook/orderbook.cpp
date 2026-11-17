#include "netrx/orderbook.hpp"

namespace netrx {
auto Orderbook::Add(const itch::OrderReferenceNumber ref, const Order order)
    -> void {
  orders_.Insert(ref, order);
  auto &levels = GetSide(order.side());
  levels[order.price()] += order.quantity();
}

auto Orderbook::ReduceOrder(const OrderIt orderIt, const Shares shares) noexcept
    -> void {
  auto &order = orderIt->second;
  order.fillOrder(shares);

  auto &side = GetSide(order.side());

  const auto levelIt = side.find(order.price());
  assert(levelIt != side.end());

  auto &levelShares = levelIt->second;
  assert(levelShares >= shares);

  levelShares -= shares;
  if (levelShares == 0) {
    side.erase(levelIt);
  }
  if (order.isFilled()) {
    orders_.Erase(orderIt);
  }
}

auto Orderbook::Execute(const itch::OrderReferenceNumber ref,
                        const Shares executedShares) noexcept -> void {
  auto *const orderIt = orders_.Find(ref);
  assert(orderIt != orders_.end());
  ReduceOrder(orderIt, executedShares);
}

auto Orderbook::Delete(const itch::OrderReferenceNumber ref) noexcept -> void {
  auto *const orderIt = orders_.Find(ref);
  assert(orderIt != orders_.end());
  ReduceOrder(orderIt, orderIt->second.quantity());
}

auto Orderbook::Cancel(const itch::OrderReferenceNumber ref,
                       const Shares cancelledShares) noexcept -> void {
  auto *const orderIt = orders_.Find(ref);
  assert(orderIt != orders_.end());
  ReduceOrder(orderIt, cancelledShares);
}

auto Orderbook::Replace(const itch::OrderReferenceNumber oldRef,
                        const itch::OrderReferenceNumber newRef,
                        const Shares shares, const Price price) -> void {
  auto const *const orderIt = orders_.Find(oldRef);
  assert(orderIt != orders_.end());
  const auto side = orderIt->second.side();
  const Order newOrder{side, price, shares};
  Add(newRef, newOrder);
  Delete(oldRef);
}

auto Orderbook::BestBid() const noexcept -> std::optional<Price> {
  if (bids_.empty()) {
    return std::nullopt;
  }
  return bids_.rbegin()->first;
}

auto Orderbook::BestAsk() const noexcept -> std::optional<Price> {
  if (asks_.empty()) {
    return std::nullopt;
  }
  return asks_.begin()->first;
}
} // namespace netrx