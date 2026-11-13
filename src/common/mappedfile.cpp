#include "netrx/mappedfile.hpp"

#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>

namespace netrx {
MappedFile::MappedFileDetails::~MappedFileDetails() noexcept {
  if (Descriptor == InvalidDescriptor)
    return;
  close(Descriptor);
}

MappedFile::MappedFile(const std::filesystem::path &path) {
  const auto descriptor = open(path.c_str(), O_RDONLY);
  if (descriptor == -1) {
    throw std::system_error(errno, std::system_category(),
                            "Cannot open file.");
  }

  // Get stats.
  struct stat sb{};
  if (fstat(descriptor, &sb) == -1) {
    throw std::system_error(errno, std::system_category(),
                            "Cannot get stats of file.");
  }

  // Open handle.
  const auto start = static_cast<char *>(
      mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, descriptor, 0));
  if (start == MAP_FAILED) {
    throw std::system_error(errno, std::system_category(),
                            "Cannot map file.");
  }

  details_ = {
      .Descriptor = descriptor,
      .Begin = start,
      .Size = static_cast<size_t>(sb.st_size),
  };

  //Prefetch Pages
  madvise(details_.Begin, details_.Size, MADV_SEQUENTIAL);
}

MappedFile::~MappedFile() {
  if (!details_.isOpen()) {
    return;
  }

  if (details_.Begin == nullptr) {
    return;
  }
  munmap(details_.Begin, details_.Size);
}
} // namespace netrx
