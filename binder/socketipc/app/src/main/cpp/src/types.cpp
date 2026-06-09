#include "ipc/types.hpp"

#include <algorithm>
#include <numeric>
#include <sstream>

namespace ipc {

std::int32_t UserProfile::checksum() const noexcept {
    std::int64_t sum = id;
    for (const char ch : name) {
        sum += static_cast<unsigned char>(ch);
    }
    for (const std::int32_t score : scores) {
        sum += score;
    }
    sum += static_cast<std::int64_t>(rating * 1000.0);
    return static_cast<std::int32_t>(sum & 0x7FFFFFFF);
}

UserProfile UserProfile::sample(const std::int64_t seed) {
    UserProfile profile;
    profile.id = seed;
    profile.name = "user-" + std::to_string(seed);
    profile.scores = {1, 2, 3, 5, 8, 13, 21};
    profile.rating = 4.5 + static_cast<double>(seed % 10) / 10.0;
    return profile;
}

UserProfile UserProfile::transformed() const {
    UserProfile out = *this;
    out.id += 1;
    out.name += "-processed";
    if (!out.scores.empty()) {
        std::transform(out.scores.begin(), out.scores.end(), out.scores.begin(),
                       [](const std::int32_t value) { return value + 1; });
    }
    out.rating += 0.1;
    return out;
}

std::string BenchmarkStats::format() const {
    std::ostringstream stream;
    stream << "success=" << success << ", failure=" << failure << ", min=" << min_ms << "ms"
           << ", avg=" << avg_ms << "ms, p95=" << p95_ms << "ms, max=" << max_ms << "ms";
    return stream.str();
}

}  // namespace ipc
