
#include <cstdint>
#include <netrx/message.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <vector>

using namespace std::chrono_literals;
constexpr size_t ITERATIONS{1000000};
constexpr auto INTERVAL{10us};

void sendPackets() {
  const auto start_time = std::chrono::steady_clock::now();
  std::chrono::nanoseconds total_lateness{};
  std::vector<int64_t> samples(ITERATIONS);

  for (size_t i{}; i < ITERATIONS; i++) {
    const auto due_time = start_time + INTERVAL * static_cast<int64_t>(i);
    while (due_time > std::chrono::steady_clock::now()) {
    }
    const auto lateness = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - due_time);
    samples[i] = lateness.count();
    total_lateness += lateness;
  }
  std::sort(samples.begin(), samples.end());
  const auto p50 = samples[static_cast<std::size_t>(samples.size() * 0.50)];
  const auto p95 = samples[static_cast<std::size_t>(samples.size() * 0.95)];
  const auto p99 = samples[static_cast<std::size_t>(samples.size() * 0.99)];
  const auto p99_9 = samples[static_cast<std::size_t>(samples.size() * 0.999)];
  const auto p_max = samples.back();

  std::cout << "-------[sendPackets] Finished-------\n";
  std::cout << "Average lateness: " << (total_lateness / ITERATIONS).count()
            << "ns\n";
  std::cout << "p50: 		  " << p50 << '\n';
  std::cout << "p95:              " << p95 << '\n';
  std::cout << "p99:              " << p99 << '\n';
  std::cout << "p99.9:            " << p99_9 << '\n';
  std::cout << "pmax:             " << p_max << '\n';
  std::cout << "------------------------------------\n";
}

int main() { sendPackets(); }
