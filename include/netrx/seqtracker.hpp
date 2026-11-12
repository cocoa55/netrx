#pragma once

#include <cstdint>

namespace netrx {
class SeqTracker {
public:
  void onPacket(const uint64_t seq) noexcept {
    if (expected_seq_ == seq) {
      expected_seq_++;
    } else if (expected_seq_ < seq) {
      const auto missing = seq - expected_seq_;
      expected_seq_ = seq + 1;
      dropped_ += missing;
    } else {
      reordered_++;
      if (dropped_ > 0)
        dropped_--;
    }
  }
  [[nodiscard]] uint64_t dropped() const noexcept { return dropped_; }
  [[nodiscard]] uint64_t reordered() const noexcept { return reordered_; }

private:
  uint64_t expected_seq_{};
  uint64_t dropped_{};
  uint64_t reordered_{};
};
} // namespace netrx
