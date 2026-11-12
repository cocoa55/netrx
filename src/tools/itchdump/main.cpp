#include <cstdint>
#include <string>

#include "netrx/bytes.hpp"
#include "netrx/itch/decoder.hpp"
#include "netrx/mappedfile.hpp"

namespace {
const std::string Filename = NETRX_ITCH_FILE;

void itchDriver() {
  const netrx::MappedFile file{Filename};

  // Parse as one big C-Style array
  auto view = file.data();
  auto begin = view.begin();
  while (begin != view.end()) {
    auto length = netrx::readBigEndian<uint16_t>(begin);
    begin += netrx::kLengthPrefixSize;
    netrx::itch::itchDecoder({begin, length});
    begin += length;
  }
}
} // namespace

int main() { itchDriver(); }
