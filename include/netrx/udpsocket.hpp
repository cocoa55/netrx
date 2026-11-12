#pragma once

#include <cerrno>
#include <sys/socket.h>
#include <unistd.h>

#include <system_error>
#include <utility>

namespace netrx {

class UdpSocket {
public:
  [[nodiscard]] UdpSocket() {
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
      throw std::system_error(errno, std::generic_category(), "socket");
    }
    fd_ = fd;
  }

  ~UdpSocket() noexcept {
    if (fd_ < 0) {
      return;
    }
    close(fd_);
  }

  UdpSocket(UdpSocket &&a) noexcept : fd_(std::exchange(a.fd_, -1)) {}

  UdpSocket &operator=(UdpSocket &&a) noexcept {
    if (this != &a) {
      if (fd_ >= 0) {
        close(fd_);
      }
      fd_ = std::exchange(a.fd_, -1);
    }
    return *this;
  }

  UdpSocket(const UdpSocket &) = delete;
  UdpSocket &operator=(const UdpSocket &) = delete;
  [[nodiscard]] int getFd() const noexcept { return fd_; }

private:
  int fd_{-1};
};
} // namespace netrx
