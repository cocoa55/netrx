#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <netrx/message.hpp>
#include <numeric>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <string_view>
using namespace std::chrono_literals;
constexpr size_t ITERATIONS{1000000};
constexpr auto INTERVAL{10us};

void printPercentiles(const std::string_view name, std::vector<std::int64_t>& samples) {
  std::ranges::sort(samples);
  const auto avg = std::accumulate(samples.begin(), samples.end(), std::int64_t{0}) / static_cast<std::int64_t>(samples.size());
  const auto p50 = samples[static_cast<std::size_t>(samples.size() * 0.50)];
  const auto p95 = samples[static_cast<std::size_t>(samples.size() * 0.95)];
  const auto p99 = samples[static_cast<std::size_t>(samples.size() * 0.99)];
  const auto p99_9 = samples[static_cast<std::size_t>(samples.size() * 0.999)];
  const auto p_max = samples.back();
  std::cout << "---"<<name<<"---\n";
  std::cout << "Average: " << avg
            << "ns\n";
  std::cout << "p50:              " << p50 << '\n';
  std::cout << "p95:              " << p95 << '\n';
  std::cout << "p99:              " << p99 << '\n';
  std::cout << "p99.9:            " << p99_9 << '\n';
  std::cout << "pmax:             " << p_max << '\n';
  std::cout << "----------------\n";
}
void sendPackets(int fd, const sockaddr_in& dest) {
  const auto start_time = std::chrono::steady_clock::now();
  std::vector<int64_t> lateness_samples(ITERATIONS);
  std::vector<int64_t> sendto_samples(ITERATIONS);
  for (size_t i{}; i < ITERATIONS; i++) {
    const auto due_time = start_time + INTERVAL * static_cast<int64_t>(i);
    while (due_time > std::chrono::steady_clock::now()) {
    }
    const auto lateness = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - due_time);
    netrx::Message msg {
      .send_time_ns = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(due_time.time_since_epoch()).count()),
      .seq_id = i,
      .order_id = 1,
      .price = 1002500,
      .quantity = 4,
      .instrument_id = 101,
      .side = netrx::Side::Ask,
      .type = netrx::MsgType::Add
    };
    auto send_start = std::chrono::steady_clock::now();
    ssize_t n = sendto(fd, &msg, sizeof(msg), 0, reinterpret_cast<const sockaddr*>(&dest), sizeof(dest));
    auto send_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - send_start);
    if (n < 0)
      std::cerr << "sendto: " << std::strerror(errno) << '\n';

    lateness_samples[i] = lateness.count();
    sendto_samples[i] = send_duration.count();
  }
  std::cout << "-------[sendPackets] Finished-------\n";
  printPercentiles("Lateness", lateness_samples);
  printPercentiles("Send Time", sendto_samples);
  std::cout << "------------------------------------\n";

}

int main() {
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    std::cerr << "socket error\n";
    return 1;
  }
  sockaddr_in dest{};
  dest.sin_family = AF_INET;
  dest.sin_port = htons(9000);
  if (inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr) != 1) {
    std::cerr << "inet_pton: invalid address " << '\n';
    close(fd);
    return 1;
  }
  sendPackets(fd, dest);
  close(fd);
}
