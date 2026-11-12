#include <cerrno>
#include <cstdint>
#include <netinet/in.h>
#include <print>
#include <sys/socket.h>

#include <charconv>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <string_view>
#include <sys/time.h>
#include <system_error>
#include <vector>

#include "netrx/config.hpp"
#include "netrx/seqtracker.hpp"
#include "netrx/stats.hpp"
#include "netrx/udpsocket.hpp"

namespace {
enum class Mode : uint8_t { Busy, Sleeping };

template <Mode M> void receivePackets(const netrx::UdpSocket &sock) {
  std::vector<int64_t> latency_samples(netrx::kIterations);
  std::vector<int64_t> pipeline_samples(netrx::kIterations);
  size_t packets_recv{};
  netrx::SeqTracker seq{};
  netrx::UdpDatagram msg{};
  std::chrono::steady_clock::time_point last_arrival{};
  constexpr auto flag = M == Mode::Busy ? MSG_DONTWAIT : 0;
  while (packets_recv < netrx::kIterations) {
    const ssize_t rec = recv(sock.getFd(), &msg, sizeof(msg), flag);
    const auto time_recv = std::chrono::steady_clock::now();
    if (rec < 0) {
      if constexpr (M == Mode::Busy) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          if (packets_recv == 0) {
            continue;
          } else if (time_recv - last_arrival >= netrx::kRecvTimeout) {
            break;
          } else {
            continue;
          }
        } else {
          throw std::system_error(errno, std::generic_category(), "recv");
        }
      }
      if constexpr (M == Mode::Sleeping) {
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
    if constexpr (M == Mode::Sleeping) {
      if (packets_recv == 1) {
        timeval tv{.tv_sec = netrx::kRecvTimeout.count(), .tv_usec = 0};
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
    std::println("No packets received");
    return;
  }
  std::println("-------[receivePackets] Finished-------");
  std::println("Packets Dropped:    {}", seq.dropped());
  std::println("Total Dropped:      {}", netrx::kIterations - packets_recv);
  std::println("Packets Reordered:  {}", seq.reordered());
  std::println("Packets Received:   {}", packets_recv);
  if constexpr (M == Mode::Busy) {
    netrx::writeSamples("results/latencyPoll.bin", latency_samples);
    netrx::writeSamples("results/pipelinePoll.bin", pipeline_samples);
  }
  if constexpr (M == Mode::Sleeping) {
    netrx::writeSamples("results/latencySleep.bin", latency_samples);
    netrx::writeSamples("results/pipelineSleep.bin", pipeline_samples);
  }
  if (const auto s = netrx::computeStats(latency_samples))
    netrx::printPercentiles("Latency", *s);
  if (const auto s = netrx::computeStats(pipeline_samples))
    netrx::printPercentiles("Pipeline", *s);
  std::println("---------------------------------------");
}
}

int main(const int argc, char *argv[]) {
  if (argc != 2 && argc != 3) {
    std::cerr << "usage: " << argv[0] << " busy|sleep [rcvbuf_bytes]\n";
    return 1;
  }
  const std::string_view arg1{argv[1]};
  Mode mode{};
  if (arg1 == "busy") {
    mode = Mode::Busy;
  } else if (arg1 == "sleep") {
    mode = Mode::Sleeping;
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
    const netrx::UdpSocket sock{};
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(netrx::kPort);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    const auto bnd = bind(sock.getFd(), reinterpret_cast<const sockaddr *>(&addr),
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
    if (mode == Mode::Busy) {
      receivePackets<Mode::Busy>(sock);
    } else {
      receivePackets<Mode::Sleeping>(sock);
    }
  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
