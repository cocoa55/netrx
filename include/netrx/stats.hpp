#pragma once
#include <cstdint>

#include <string_view>
#include <vector>

namespace stats {
void printPercentiles(const std::string_view name,
                      std::vector<int64_t> &samples);
};
