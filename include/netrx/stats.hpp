#pragma once
#include <cstdint>

#include <filesystem>
#include <string_view>
#include <vector>

namespace stats {
void printPercentiles(const std::string_view name,
                      std::vector<int64_t> &samples);
void writeSamples(const std::filesystem::path &path,
                  const std::vector<int64_t> &samples);
} // namespace stats
