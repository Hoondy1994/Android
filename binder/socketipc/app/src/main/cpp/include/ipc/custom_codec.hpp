#pragma once

#include "ipc/types.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace ipc {

inline constexpr std::uint32_t kCustomMagic = 0x43504943;  // "IPCC"

class CustomCodec {
public:
    [[nodiscard]] static std::vector<std::uint8_t> encode(const UserProfile& profile);
    [[nodiscard]] static UserProfile decode(std::span<const std::uint8_t> data);
    [[nodiscard]] static std::vector<std::uint8_t> process(std::span<const std::uint8_t> data);
};

}  // namespace ipc
