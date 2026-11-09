#include <cstdint>
#include <netrx/stats.hpp>
#include <print>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>

namespace stats {
void writeSamples(const std::filesystem::path &path,
                  std::span<int64_t> samples) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("cannot open " + path.string());
  }
  out.write(reinterpret_cast<const char *>(samples.data()),
            static_cast<std::streamsize>(samples.size() * sizeof(int64_t)));
  if (!out) {
    throw std::runtime_error("write failed: " + path.string());
  }
}
std::optional<LatencyStats> computeStats(std::span<int64_t> samples) {
  if (samples.empty()) {
    return std::nullopt;
  }
  std::ranges::sort(samples);
  LatencyStats stats{
      .avg = std::ranges::fold_left(samples, int64_t{0}, std::plus{}) /
             static_cast<int64_t>(samples.size()),
      .p50 = samples[static_cast<size_t>(samples.size() * 0.50)],
      .p95 = samples[static_cast<size_t>(samples.size() * 0.95)],
      .p99 = samples[static_cast<size_t>(samples.size() * 0.99)],
      .p99_9 = samples[static_cast<size_t>(samples.size() * 0.999)],
      .pmax = samples.back(),
      .count = samples.size()};
  return stats;
}
void printPercentiles(const std::string_view name, const LatencyStats &stats) {
  std::println("---{}---", name);
  std::println("Average:           {}ns", stats.avg);
  std::println("p50:               {}ns", stats.p50);
  std::println("p95:               {}ns", stats.p95);
  std::println("p99:               {}ns", stats.p99);
  std::println("p99.9:             {}ns", stats.p99_9);
  std::println("pmax:              {}ns", stats.pmax);
  std::println("count:             {} packets", stats.count);
  std::println("----------------");
}
} // namespace stats
