#pragma once

#include "ipc/socket_protocol.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace ipc {

class SocketClient {
public:
    explicit SocketClient(std::string socket_name);

    [[nodiscard]] bool ping();
    [[nodiscard]] std::int32_t add(std::int32_t a, std::int32_t b);
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> process_custom(
        std::span<const std::uint8_t> payload);
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> process_parcel(
        std::span<const std::uint8_t> payload);

private:
    [[nodiscard]] std::optional<Response> transact(const Frame& frame);

    std::string socket_name_;
};

[[nodiscard]] SocketClient& global_socket_client();

}  // namespace ipc
