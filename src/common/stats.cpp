#include <cstdint>
#include <netrx/stats.hpp>
#include <print>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string_view>

namespace stats {
void writeSamples(const std::filesystem::path &path,
                  const std::vector<int64_t> &samples) {
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
void printPercentiles(const std::string_view name,
                      std::vector<int64_t> &samples) {
  std::ranges::sort(samples);
  const auto avg = std::accumulate(samples.begin(), samples.end(), int64_t{0}) /
                   static_cast<int64_t>(samples.size());
  const auto p50 = samples[static_cast<std::size_t>(samples.size() * 0.50)];
  const auto p95 = samples[static_cast<std::size_t>(samples.size() * 0.95)];
  const auto p99 = samples[static_cast<std::size_t>(samples.size() * 0.99)];
  const auto p99_9 = samples[static_cast<std::size_t>(samples.size() * 0.999)];
  const auto p_max = samples.back();
  std::println("---{}---", name);
  std::println("Average:           {}ns", avg);
  std::println("p50:               {}", p50);
  std::println("p95:               {}", p95);
  std::println("p99:               {}", p99);
  std::println("p99.9:             {}", p99_9);
  std::println("pmax:              {}", p_max);
  std::println("----------------");
}
} // namespace stats
