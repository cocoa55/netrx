#include <cerrno>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <sys/time.h>
#include <system_error>
#include <vector>

#include "netrx/config.hpp"
#include "netrx/message.hpp"
#include "netrx/stats.hpp"

void receivePackets(int fd) {
  std::vector<int64_t> latency_samples(netrx::ITERATIONS);
  size_t packets_recv{};
  uint64_t expected_seq{};
  size_t packets_dropped{};
  size_t packets_reordered{};
  netrx::Message msg;
  while (packets_recv < netrx::ITERATIONS) {
    const ssize_t rec = recv(fd, &msg, sizeof(msg), 0);
    if (rec < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;
      throw std::system_error(errno, std::generic_category(), "recv");
    }
    if (static_cast<std::size_t>(rec) != sizeof(msg))
      continue;
    const auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::steady_clock::now().time_since_epoch())
                            .count();
    latency_samples[packets_recv] =
        now_ns - static_cast<int64_t>(msg.send_time_ns);
    packets_recv++;
    if (packets_recv == 1) {
      timeval tv{.tv_sec = netrx::RECV_TIMEOUT.count(), .tv_usec = 0};
      if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        throw std::system_error(errno, std::generic_category(), "setsockopt");
    }

    if (expected_seq == msg.seq_id) {
      expected_seq++;
    } else if (expected_seq < msg.seq_id) {
      const auto missing_id = msg.seq_id - expected_seq;
      expected_seq = msg.seq_id + 1;
      packets_dropped += missing_id;
    } else {
      packets_reordered++;
      if (packets_dropped > 0)
        packets_dropped--;
    }
  }
  latency_samples.resize(packets_recv);
  if (packets_recv == 0) {
    std::cout << "No packets recieved\n";
    return;
  }
  std::cout << "-------[receivePackets] Finished-------\n";
  std::cout << "Packets Dropped:   " << packets_dropped << '\n';
  std::cout << "Total Dropped:     " << netrx::ITERATIONS - packets_recv
            << '\n';
  std::cout << "Packets Reordered: " << packets_reordered << '\n';
  std::cout << "Packets Received:  " << packets_recv << '\n';
  stats::printPercentiles("Latency", latency_samples);
  std::cout << "---------------------------------------\n";
}

int main() {
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    std::cerr << "socket err " << std::strerror(errno) << "\n";
    return 1;
  }
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(netrx::PORT);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  auto bnd = bind(fd, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
  if (bnd < 0) {
    std::cerr << "bind err " << std::strerror(errno) << "\n";

    close(fd);
    return 1;
  }
  try {
    receivePackets(fd);
  } catch (const std::system_error &e) {
    std::cerr << e.what() << "\n";
  }
  close(fd);
}
