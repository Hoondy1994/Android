#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ipc {

struct UserProfile {
    std::int64_t id{};
    std::string name;
    std::vector<std::int32_t> scores;
    double rating{};

    [[nodiscard]] std::int32_t checksum() const noexcept;
    [[nodiscard]] static UserProfile sample(std::int64_t seed);
    [[nodiscard]] UserProfile transformed() const;
};

struct BenchmarkStats {
    double min_ms{};
    double max_ms{};
    double avg_ms{};
    double p95_ms{};
    std::size_t success{};
    std::size_t failure{};

    [[nodiscard]] std::string format() const;
};

}  // namespace ipc
