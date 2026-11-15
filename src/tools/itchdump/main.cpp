#include <print>
#include <string>

#include "netrx/itch/parser.hpp"
#include "netrx/mappedfile.hpp"

namespace netrx::itch {
const std::string Filename = NETRX_ITCH_FILE;
} // namespace netrx::itch

int main() {
  const netrx::MappedFile file{netrx::itch::Filename};

  const auto counts = netrx::itch::parseItch(file.data());
  if (!counts.has_value()) {
    return 1;
  }
  if (counts->lastEventCode != 'C') {
    std::println(stderr, "Incorrect Last event, Should be 'C', is '{}'",
                 counts->lastEventCode);
    return 1;
  }
  return 0;
}
