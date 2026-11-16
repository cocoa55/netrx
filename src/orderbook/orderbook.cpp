#include "netrx/orderbook.hpp"

namespace netrx {
void Orderbook::Add(const itch::OrderReferenceNumber ref, const Order order) {
  // Check that this order doesn't already exist.
  [[maybe_unused]] const bool added = orders_.insert({ref, order}).second;
  assert(added);

  auto &levels = GetSide(order.side());
  levels[order.price()] += order.quantity();
}

void Orderbook::ReduceOrder(const OrderIt orderIt, const Shares shares) {
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
    orders_.erase(orderIt);
  }
}

void Orderbook::Execute(const itch::OrderReferenceNumber ref,
                        const Shares executedShares) {
  const auto orderIt = orders_.find(ref);
  assert(orderIt != orders_.end());
  ReduceOrder(orderIt, executedShares);
}

void Orderbook::Delete(const itch::OrderReferenceNumber ref) {
  const auto orderIt = orders_.find(ref);
  assert(orderIt != orders_.end());
  ReduceOrder(orderIt, orderIt->second.quantity());
}

void Orderbook::Cancel(const itch::OrderReferenceNumber ref,
                       const Shares cancelledShares) {
  const auto orderIt = orders_.find(ref);
  assert(orderIt != orders_.end());
  ReduceOrder(orderIt, cancelledShares);
}

void Orderbook::Replace(const itch::OrderReferenceNumber oldRef,
                        const itch::OrderReferenceNumber newRef,
                        const Shares shares, const Price price) {
  const auto orderIt = orders_.find(oldRef);
  assert(orderIt != orders_.end());
  const auto side = orderIt->second.side();
  const Order newOrder{side, price, shares};
  Add(newRef, newOrder);
  Delete(oldRef);
}

auto Orderbook::BestBid() const -> std::optional<Price> {
  if (bids_.empty()) {
    return std::nullopt;
  }
  return bids_.rbegin()->first;
}

auto Orderbook::BestAsk() const -> std::optional<Price> {
  if (asks_.empty()) {
    return std::nullopt;
  }
  return asks_.begin()->first;
}
} // namespace netrx