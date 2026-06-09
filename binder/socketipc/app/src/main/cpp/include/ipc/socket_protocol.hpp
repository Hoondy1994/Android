#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace ipc {

enum class Opcode : std::uint8_t {
    kAdd = 1,
    kProcessCustom = 2,
    kProcessParcel = 3,
    kPing = 4,
};

enum class Status : std::uint8_t {
    kOk = 0,
    kError = 1,
};

struct Frame {
    Opcode opcode{};
    std::vector<std::uint8_t> payload;
};

struct Response {
    Status status{Status::kError};
    std::vector<std::uint8_t> payload;
};

class SocketProtocol {
public:
    [[nodiscard]] static std::vector<std::uint8_t> encode_request(const Frame& frame);
    [[nodiscard]] static Frame decode_request(std::span<const std::uint8_t> data);

    [[nodiscard]] static std::vector<std::uint8_t> encode_response(const Response& response);
    [[nodiscard]] static Response decode_response(std::span<const std::uint8_t> data);

    [[nodiscard]] static std::vector<std::uint8_t> read_fully(int fd);
    [[nodiscard]] static bool write_fully(int fd, std::span<const std::uint8_t> data);
};

}  // namespace ipc
