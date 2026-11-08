#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <netinet/in.h>
#include <print>
#include <sys/socket.h>
#include <unistd.h>

#include <charconv>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <string_view>
#include <sys/time.h>
#include <system_error>
#include <vector>

#include "netrx/config.hpp"
#include "netrx/message.hpp"
#include "netrx/seq_tracker.hpp"
#include "netrx/socket.hpp"
#include "netrx/stats.hpp"

enum class Mode : uint8_t { BUSY, SLEEPING };
template <Mode M> void receivePackets(const netrx::Socket &sock) {
  std::vector<int64_t> latency_samples(netrx::ITERATIONS);
  std::vector<int64_t> pipeline_samples(netrx::ITERATIONS);
  size_t packets_recv{};
  netrx::SeqTracker seq{};
  netrx::Message msg;
  std::chrono::steady_clock::time_point last_arrival{};
  constexpr auto flag = M == Mode::BUSY ? MSG_DONTWAIT : 0;
  while (packets_recv < netrx::ITERATIONS) {
    const ssize_t rec = recv(sock.getFd(), &msg, sizeof(msg), flag);
    const auto time_recv = std::chrono::steady_clock::now();
    if (rec < 0) {
      if constexpr (M == Mode::BUSY) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          if (packets_recv == 0) {
            continue;
          } else if (time_recv - last_arrival >= netrx::RECV_TIMEOUT) {
            break;
          } else {
            continue;
          }
        } else {
          throw std::system_error(errno, std::generic_category(), "recv");
        }
      }
      if constexpr (M == Mode::SLEEPING) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
          break;
        throw std::system_error(errno, std::generic_category(), "recv");
      }
    }
    last_arrival = time_recv;
    if (static_cast<std::size_t>(rec) != sizeof(msg))
      continue;
    const auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            time_recv.time_since_epoch())
                            .count();
    latency_samples[packets_recv] =
        now_ns - static_cast<int64_t>(msg.intended_send_ns);
    pipeline_samples[packets_recv] =
        now_ns - static_cast<int64_t>(msg.actual_send_ns);

    packets_recv++;
    if constexpr (M == Mode::SLEEPING) {
      if (packets_recv == 1) {
        timeval tv{.tv_sec = netrx::RECV_TIMEOUT.count(), .tv_usec = 0};
        if (setsockopt(sock.getFd(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) <
            0)
          throw std::system_error(errno, std::generic_category(), "setsockopt");
      }
    }
    seq.onPacket(msg.seq_id);
  }
  latency_samples.resize(packets_recv);
  pipeline_samples.resize(packets_recv);
  if (packets_recv == 0) {
    std::cout << "No packets received\n";
    return;
  }
  std::println("-------[receivePackets] Finished-------");
  std::println("Packets Dropped:    {}", seq.dropped());
  std::println("Total Dropped:      {}", netrx::ITERATIONS - packets_recv);
  std::println("Packets Reordered:  {}", seq.reordered());
  std::println("Packets Received:   {}", packets_recv);
  if constexpr (M == Mode::BUSY) {
    stats::writeSamples("results/latencyPoll.bin", latency_samples);
    stats::writeSamples("results/pipelinePoll.bin", pipeline_samples);
  }
  if constexpr (M == Mode::SLEEPING) {
    stats::writeSamples("results/latencySleep.bin", latency_samples);
    stats::writeSamples("results/pipelineSleep.bin", pipeline_samples);
  }
  stats::printPercentiles("Latency", latency_samples);
  stats::printPercentiles("Pipeline", pipeline_samples);
  std::println("---------------------------------------");
}

int main(int argc, char *argv[]) {
  if (argc != 2 && argc != 3) {
    std::cerr << "usage: " << argv[0] << " busy|sleep [rcvbuf_bytes]\n";
    return 1;
  }
  const std::string_view arg1{argv[1]};
  Mode mode{};
  if (arg1 == "busy") {
    mode = Mode::BUSY;
  } else if (arg1 == "sleep") {
    mode = Mode::SLEEPING;
  } else {
    std::cerr << "unknown mode '" << arg1 << "'. usage: " << argv[0]
              << " busy|sleep [rcvbuf_bytes]\n";
    return 1;
  }

  int rcvbuf{};
  if (argc == 3) {
    const std::string_view size_arg{argv[2]};
    const auto [ptr, ec] = std::from_chars(
        size_arg.data(), size_arg.data() + size_arg.size(), rcvbuf);
    if (ec != std::errc{} || ptr != size_arg.data() + size_arg.size() ||
        rcvbuf <= 0) {
      std::cerr << "invalid rcvbuf size '" << size_arg
                << "'. usage: " << argv[0] << " busy|sleep [rcvbuf_bytes]\n";
      return 1;
    }
  }
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
    if (rcvbuf > 0) {
      if (setsockopt(sock.getFd(), SOL_SOCKET, SO_RCVBUF, &rcvbuf,
                     sizeof(rcvbuf)) < 0) {
        throw std::system_error(errno, std::generic_category(), "setsockopt");
      }
    }
    int actual{};
    socklen_t len = sizeof(actual);
    if (getsockopt(sock.getFd(), SOL_SOCKET, SO_RCVBUF, &actual, &len) < 0) {
      throw std::system_error(errno, std::generic_category(), "getsockopt");
    }

    std::println("rcvbuf: {} bytes", actual);
    if (mode == Mode::BUSY) {
      receivePackets<Mode::BUSY>(sock);
    } else {
      receivePackets<Mode::SLEEPING>(sock);
    }
  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
