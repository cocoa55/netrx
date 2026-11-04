#include <cerrno>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>

#include <chrono>
#include <iostream>
#include <sys/time.h>
#include <system_error>
#include <vector>

#include "netrx/config.hpp"
#include "netrx/message.hpp"
#include "netrx/seq_tracker.hpp"
#include "netrx/socket.hpp"
#include "netrx/stats.hpp"

void receivePackets(const netrx::Socket &sock) {
  std::vector<int64_t> latency_samples(netrx::ITERATIONS);
  size_t packets_recv{};
  netrx::SeqTracker seq{};
  netrx::Message msg;
  while (packets_recv < netrx::ITERATIONS) {
    const ssize_t rec = recv(sock.getFd(), &msg, sizeof(msg), 0);
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
      if (setsockopt(sock.getFd(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) <
          0)
        throw std::system_error(errno, std::generic_category(), "setsockopt");
    }
    seq.onPacket(msg.seq_id);
  }
  latency_samples.resize(packets_recv);
  if (packets_recv == 0) {
    std::cout << "No packets received\n";
    return;
  }
  std::cout << "-------[receivePackets] Finished-------\n";
  std::cout << "Packets Dropped:   " << seq.dropped() << '\n';
  std::cout << "Total Dropped:     " << netrx::ITERATIONS - packets_recv
            << '\n';
  std::cout << "Packets Reordered: " << seq.reordered() << '\n';
  std::cout << "Packets Received:  " << packets_recv << '\n';
  stats::writeSamples("results/latency.bin", latency_samples);
  stats::printPercentiles("Latency", latency_samples);
  std::cout << "---------------------------------------\n";
}

int main() {
  try {
    netrx::Socket sock{};
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(netrx::PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    auto bnd = bind(sock.getFd(), reinterpret_cast<const sockaddr *>(&addr),
                    sizeof(addr));
    if (bnd < 0) {
      throw std::system_error(errno, std::generic_category(), "bind");
    }
    receivePackets(sock);
  } catch (const std::system_error &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
