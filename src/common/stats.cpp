#include <netrx/stats.hpp>

#include <algorithm>
#include <iostream>
#include <numeric>

namespace stats {
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
  std::cout << "---" << name << "---\n";
  std::cout << "Average: " << avg << "ns\n";
  std::cout << "p50:              " << p50 << '\n';
  std::cout << "p95:              " << p95 << '\n';
  std::cout << "p99:              " << p99 << '\n';
  std::cout << "p99.9:            " << p99_9 << '\n';
  std::cout << "pmax:             " << p_max << '\n';
  std::cout << "----------------\n";
}
}; // namespace stats
