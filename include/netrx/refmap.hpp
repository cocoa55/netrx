#pragma once

#include <bit>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

namespace netrx {

template <std::unsigned_integral Key, std::default_initializable Value,
          size_t LoadNum = 1, size_t LoadDen = 2>
class RefMap {
  static_assert(LoadNum > 0 && LoadNum < LoadDen);

  struct Slot {
    Key first{};
    Value second{};
  };

public:
  using iterator = Slot *;
  using const_iterator = const Slot *;

  // capacity must be a power of two (std::has_single_bit). All keys start at 0.
  explicit RefMap(size_t initialCapacity)
      : slots_(initialCapacity), mask_{initialCapacity - 1} {
    assert(std::has_single_bit(initialCapacity));
  }

  // Grow first if this insert would pass the max load.
  // Walk from Home(key) to the first empty slot, place it, ++size_.
  auto Insert(Key key, Value value) -> void {
    assert(key != 0);
    assert(Find(key) == end());
    if (ShouldGrow())
      Grow();

    Place(key, std::move(value));
    ++size_;
  }

  [[nodiscard]] iterator Find(Key key) noexcept {
    for (auto i{Home(key)};; i = Next(i)) {
      if (slots_[i].first == key)
        return &slots_[i];
      if (slots_[i].first == 0)
        return nullptr;
    }
  }

  [[nodiscard]] const_iterator Find(Key key) const noexcept {
    for (auto i{Home(key)};; i = Next(i)) {
      if (slots_[i].first == key)
        return &slots_[i];
      if (slots_[i].first == 0)
        return nullptr;
    }
  }

  // Find the slot (assert it exists), empty it -> hole, then backward shift:
  // walk from Next(hole) until an empty slot; move entry j into the hole when
  // DistanceFromHome(j) >= ((j - hole) & mask_), and j becomes the new hole.
  // --size_. Any Value* from Find is stale after this.
  void Erase(iterator it) noexcept(std::is_nothrow_move_assignable_v<Value>) {
    assert(it != end());
    auto hole = static_cast<size_t>(it - slots_.data());
    slots_[hole].first = 0;
    for (auto j{Next(hole)};; j = Next(j)) {
      if (slots_[j].first == 0) {
        break;
      }
      if (DistanceFromHome(j) >= ((j - hole) & mask_)) {
        slots_[hole] = std::move(slots_[j]);
        slots_[j].first = 0;
        hole = j;
      }
    }
    --size_;
  }
  [[nodiscard]] constexpr auto Size() const noexcept -> size_t { return size_; }
  [[nodiscard]] constexpr auto Capacity() const noexcept -> size_t {
    return slots_.size();
  }
  [[nodiscard]] constexpr auto GrowCount() const noexcept -> size_t {
    return growCount_;
  }

  [[nodiscard]] constexpr auto end() const noexcept -> Slot * {
    return nullptr;
  }

private:
  [[nodiscard]] static constexpr auto Hash(Key key) noexcept -> size_t {
    return key;
  }

  [[nodiscard]] constexpr auto Home(Key key) const noexcept -> size_t {
    return Hash(key) & mask_;
  }

  [[nodiscard]] constexpr auto Next(const size_t i) const noexcept -> size_t {
    return (i + 1) & mask_;
  }

  [[nodiscard]] constexpr auto DistanceFromHome(size_t i) const noexcept
      -> size_t {
    return (i - Home(slots_[i].first)) & mask_;
  }

  [[nodiscard]] constexpr auto ShouldGrow() const noexcept -> bool {
    return (size_ + 1) * LoadDen > Capacity() * LoadNum;
  }

  auto Grow() -> void {
    auto old = std::move(slots_);
    slots_ = std::vector<Slot>(old.size() * 2);
    mask_ = Capacity() - 1;
    for (auto &slot :
         old | std::views::filter([](const Slot &s) { return s.first != 0; })) {
      Place(slot.first, std::move(slot.second));
    }
    ++growCount_;
  }

  auto Place(Key key,
             Value value) noexcept(std::is_nothrow_move_assignable_v<Value>)
      -> void {
    for (auto i = Home(key);; i = Next(i)) {
      if (slots_[i].first == 0) {
        slots_[i] = {.first = key, .second = std::move(value)};
        break;
      }
    }
  }

  std::vector<Slot> slots_;
  size_t mask_{}; // capacity - 1
  size_t size_{};
  size_t growCount_{};
};

} // namespace netrx