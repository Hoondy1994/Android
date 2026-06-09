#include "ipc/socket_protocol.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <unistd.h>

namespace ipc {

namespace {

void append_u32(std::vector<std::uint8_t>& out, const std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>(value & 0xFF));
}

[[nodiscard]] std::uint32_t read_u32(std::span<const std::uint8_t> data, std::size_t& offset) {
    if (offset + 4 > data.size()) {
        throw std::runtime_error("protocol frame too short");
    }
    const std::uint32_t value =
        (static_cast<std::uint32_t>(data[offset]) << 24) |
        (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
        (static_cast<std::uint32_t>(data[offset + 2]) << 8) |
        static_cast<std::uint32_t>(data[offset + 3]);
    offset += 4;
    return value;
}

}  // namespace

std::vector<std::uint8_t> SocketProtocol::encode_request(const Frame& frame) {
    std::vector<std::uint8_t> out;
    out.reserve(5 + frame.payload.size());
    out.push_back(static_cast<std::uint8_t>(frame.opcode));
    append_u32(out, static_cast<std::uint32_t>(frame.payload.size()));
    out.insert(out.end(), frame.payload.begin(), frame.payload.end());
    return out;
}

Frame SocketProtocol::decode_request(const std::span<const std::uint8_t> data) {
    if (data.size() < 5) {
        throw std::runtime_error("request frame too short");
    }
    std::size_t offset = 0;
    Frame frame;
    frame.opcode = static_cast<Opcode>(data[offset++]);
    const auto payload_size = read_u32(data, offset);
    if (offset + payload_size > data.size()) {
        throw std::runtime_error("request payload truncated");
    }
    frame.payload.assign(data.begin() + static_cast<std::ptrdiff_t>(offset),
                         data.begin() + static_cast<std::ptrdiff_t>(offset + payload_size));
    return frame;
}

std::vector<std::uint8_t> SocketProtocol::encode_response(const Response& response) {
    std::vector<std::uint8_t> out;
    out.reserve(5 + response.payload.size());
    out.push_back(static_cast<std::uint8_t>(response.status));
    append_u32(out, static_cast<std::uint32_t>(response.payload.size()));
    out.insert(out.end(), response.payload.begin(), response.payload.end());
    return out;
}

Response SocketProtocol::decode_response(const std::span<const std::uint8_t> data) {
    if (data.size() < 5) {
        throw std::runtime_error("response frame too short");
    }
    std::size_t offset = 0;
    Response response;
    response.status = static_cast<Status>(data[offset++]);
    const auto payload_size = read_u32(data, offset);
    if (offset + payload_size > data.size()) {
        throw std::runtime_error("response payload truncated");
    }
    response.payload.assign(data.begin() + static_cast<std::ptrdiff_t>(offset),
                            data.begin() + static_cast<std::ptrdiff_t>(offset + payload_size));
    return response;
}

std::vector<std::uint8_t> SocketProtocol::read_fully(const int fd) {
    std::uint8_t header[5];
    std::size_t read_total = 0;
    while (read_total < sizeof(header)) {
        const auto n = ::read(fd, header + read_total, sizeof(header) - read_total);
        if (n <= 0) {
            throw std::runtime_error("socket read failed");
        }
        read_total += static_cast<std::size_t>(n);
    }

    const std::size_t payload_size =
        (static_cast<std::size_t>(header[1]) << 24) |
        (static_cast<std::size_t>(header[2]) << 16) |
        (static_cast<std::size_t>(header[3]) << 8) |
        static_cast<std::size_t>(header[4]);

    std::vector<std::uint8_t> buffer(sizeof(header) + payload_size);
    std::memcpy(buffer.data(), header, sizeof(header));
    read_total = 0;
    while (read_total < payload_size) {
        const auto n = ::read(fd, buffer.data() + sizeof(header) + read_total, payload_size - read_total);
        if (n <= 0) {
            throw std::runtime_error("socket payload read failed");
        }
        read_total += static_cast<std::size_t>(n);
    }
    return buffer;
}

bool SocketProtocol::write_fully(const int fd, const std::span<const std::uint8_t> data) {
    std::size_t written = 0;
    while (written < data.size()) {
        const auto n = ::write(fd, data.data() + written, data.size() - written);
        if (n <= 0) {
            return false;
        }
        written += static_cast<std::size_t>(n);
    }
    return true;
}

}  // namespace ipc
