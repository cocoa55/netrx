#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <print>
#include <sys/socket.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "netrx/config.hpp"
#include "netrx/message.hpp"
#include "netrx/socket.hpp"
#include "netrx/stats.hpp"

void sendPackets(const netrx::Socket &sock, const sockaddr_in &dest) {
  std::vector<int64_t> lateness_samples(netrx::kIterations);
  std::vector<int64_t> sendto_samples(netrx::kIterations);
  const auto start_time = std::chrono::steady_clock::now();
  for (size_t i{}; i < netrx::kIterations; i++) {
    const auto due_time =
        start_time + netrx::kInterval * static_cast<int64_t>(i);
    while (due_time > std::chrono::steady_clock::now()) {
    }
    const auto actual_send = std::chrono::steady_clock::now();
    const auto lateness = std::chrono::duration_cast<std::chrono::nanoseconds>(
        actual_send - due_time);
    netrx::Message msg{.intended_send_ns = static_cast<uint64_t>(
                           std::chrono::duration_cast<std::chrono::nanoseconds>(
                               due_time.time_since_epoch())
                               .count()),
                       .actual_send_ns = static_cast<uint64_t>(
                           std::chrono::duration_cast<std::chrono::nanoseconds>(
                               actual_send.time_since_epoch())
                               .count()),
                       .seq_id = i,
                       .order_id = 1,
                       .price = 1002500,
                       .quantity = 4,
                       .instrument_id = 101,
                       .side = netrx::Side::Ask,
                       .type = netrx::MsgType::Add};
    auto send_start = std::chrono::steady_clock::now();
    const ssize_t n = sendto(sock.getFd(), &msg, sizeof(msg), 0,
                       reinterpret_cast<const sockaddr *>(&dest), sizeof(dest));
    auto send_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - send_start);
    if (n < 0)
      std::cerr << "sendto: " << std::strerror(errno) << '\n';

    lateness_samples[i] = lateness.count();
    sendto_samples[i] = send_duration.count();
  }
  std::println("-------[sendPackets] Finished-------");
  netrx::writeSamples("results/lateness.bin", lateness_samples);
  netrx::writeSamples("results/sendto.bin", sendto_samples);
  if (const auto s = netrx::computeStats(lateness_samples))
    netrx::printPercentiles("Lateness", *s);
  if (const auto s = netrx::computeStats(sendto_samples))
    netrx::printPercentiles("Send Time", *s);

  std::println("------------------------------------");
}

int main() {
  try {
    const netrx::Socket sock{};
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(netrx::kPort);
    if (inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr) != 1) {
      std::cerr << "inet_pton: invalid address " << '\n';
      return 1;
    }
    sendPackets(sock, dest);
  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
  return 0;
}
