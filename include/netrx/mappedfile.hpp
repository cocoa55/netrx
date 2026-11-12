#pragma once

#include <cstddef>
#include <filesystem>
#include <span>

namespace netrx {
struct MappedFile {
  struct MappedFileDetails {
    ~MappedFileDetails() noexcept;

    [[nodiscard]] bool isOpen() const {
      return Descriptor != InvalidDescriptor;
    }

    constexpr static int InvalidDescriptor = -1;
    int Descriptor{InvalidDescriptor};
    char *Begin{nullptr};
    size_t Size{0};
  };

  explicit MappedFile(const std::filesystem::path &path);
  ~MappedFile();

  [[nodiscard]] std::span<const char> data() const {
    return {details_.Begin, details_.Size};
  }

  MappedFileDetails details_{};
};
} // namespace netrx
