#pragma once
#include <cstdint>

#include <filesystem>
#include <optional>
#include <span>
#include <string_view>

namespace netrx {
struct LatencyStats {
  int64_t avg{};
  int64_t p50{};
  int64_t p95{};
  int64_t p99{};
  int64_t p99_9{};
  int64_t pmax{};
  size_t count{};
};
std::optional<LatencyStats> computeStats(std::span<int64_t> samples);
void printPercentiles(std::string_view name, const LatencyStats &);
void writeSamples(const std::filesystem::path &path,
                  std::span<int64_t> samples);
} // namespace stats
