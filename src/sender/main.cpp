#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstddef>
#include <iostream>
#include <string_view>
#include <vector>

#include "netrx/config.hpp"
#include "netrx/message.hpp"
#include "netrx/stats.hpp"

void sendPackets(int fd, const sockaddr_in &dest) {
  const auto start_time = std::chrono::steady_clock::now();
  std::vector<int64_t> lateness_samples(netrx::ITERATIONS);
  std::vector<int64_t> sendto_samples(netrx::ITERATIONS);
  for (size_t i{}; i < netrx::ITERATIONS; i++) {
    const auto due_time =
        start_time + netrx::INTERVAL * static_cast<int64_t>(i);
    while (due_time > std::chrono::steady_clock::now()) {
    }
    const auto lateness = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - due_time);
    netrx::Message msg{.send_time_ns = static_cast<uint64_t>(
                           std::chrono::duration_cast<std::chrono::nanoseconds>(
                               due_time.time_since_epoch())
                               .count()),
                       .seq_id = i,
                       .order_id = 1,
                       .price = 1002500,
                       .quantity = 4,
                       .instrument_id = 101,
                       .side = netrx::Side::Ask,
                       .type = netrx::MsgType::Add};
    auto send_start = std::chrono::steady_clock::now();
    ssize_t n = sendto(fd, &msg, sizeof(msg), 0,
                       reinterpret_cast<const sockaddr *>(&dest), sizeof(dest));
    auto send_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - send_start);
    if (n < 0)
      std::cerr << "sendto: " << std::strerror(errno) << '\n';

    lateness_samples[i] = lateness.count();
    sendto_samples[i] = send_duration.count();
  }
  std::cout << "-------[sendPackets] Finished-------\n";
  stats::printPercentiles("Lateness", lateness_samples);
  stats::printPercentiles("Send Time", sendto_samples);
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
  dest.sin_port = htons(netrx::PORT);
  if (inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr) != 1) {
    std::cerr << "inet_pton: invalid address " << '\n';
    close(fd);
    return 1;
  }
  sendPackets(fd, dest);
  close(fd);
}
