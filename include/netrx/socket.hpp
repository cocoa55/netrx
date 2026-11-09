#pragma once

#include <cerrno>
#include <sys/socket.h>
#include <unistd.h>

#include <system_error>
#include <utility>

namespace netrx {

class Socket {
public:
  [[nodiscard]] Socket() {
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
      throw std::system_error(errno, std::generic_category(), "socket");
    }
    fd_ = fd;
  }

  ~Socket() noexcept {
    if (fd_ < 0) {
      return;
    }
    close(fd_);
  }

  Socket(Socket &&a) noexcept : fd_(std::exchange(a.fd_, -1)) {}
  Socket &operator=(Socket &&a) noexcept {
    if (this != &a) {
      if (fd_ >= 0) {
        close(fd_);
      }
      fd_ = std::exchange(a.fd_, -1);
    }
    return *this;
  }

  Socket(const Socket &) = delete;
  Socket &operator=(const Socket &) = delete;
  [[nodiscard]] int getFd() const noexcept { return fd_; }

private:
  int fd_{-1};
};
} // namespace netrx
