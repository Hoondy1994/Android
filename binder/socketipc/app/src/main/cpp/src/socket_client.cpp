#include "ipc/socket_client.hpp"

#include <android/log.h>

#include <cstring>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#define LOG_TAG "SocketIPC"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace ipc {

namespace {

[[nodiscard]] bool setup_abstract_address(const std::string& socket_name, sockaddr_un* addr,
                                          socklen_t* len) {
    if (socket_name.empty() || socket_name.size() >= sizeof(addr->sun_path) - 1) {
        return false;
    }
    std::memset(addr, 0, sizeof(*addr));
    addr->sun_family = AF_UNIX;
    addr->sun_path[0] = '\0';
    std::strncpy(addr->sun_path + 1, socket_name.c_str(), sizeof(addr->sun_path) - 2);
    *len = offsetof(sockaddr_un, sun_path) + 1 + socket_name.size();
    return true;
}

}  // namespace

SocketClient::SocketClient(std::string socket_name) : socket_name_(std::move(socket_name)) {}

std::optional<Response> SocketClient::transact(const Frame& frame) {
    LOGI("[Flow:SocketClient] transact opcode=%d, pid=%d", static_cast<int>(frame.opcode), getpid());
    const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        LOGE("Failed to create client socket");
        return std::nullopt;
    }

    sockaddr_un addr {};
    socklen_t addr_len = 0;
    if (!setup_abstract_address(socket_name_, &addr, &addr_len)) {
        close(fd);
        return std::nullopt;
    }

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), addr_len) < 0) {
        LOGE("[Flow:SocketClient] connect 失败");
        close(fd);
        return std::nullopt;
    }
    LOGI("[Flow:SocketClient] connect 成功, fd=%d", fd);

    const auto encoded_request = SocketProtocol::encode_request(frame);
    if (!SocketProtocol::write_fully(fd, encoded_request)) {
        close(fd);
        LOGE("Failed to write socket request");
        return std::nullopt;
    }

    try {
        const auto encoded_response = SocketProtocol::read_fully(fd);
        close(fd);
        const auto response = SocketProtocol::decode_response(encoded_response);
        LOGI("[Flow:SocketClient] transact 完成 opcode=%d, status=%d",
             static_cast<int>(frame.opcode), static_cast<int>(response.status));
        return response;
    } catch (const std::exception& ex) {
        close(fd);
        LOGE("Failed to read socket response: %s", ex.what());
        return std::nullopt;
    }
}

bool SocketClient::ping() {
    const auto response = transact(Frame{.opcode = Opcode::kPing});
    return response.has_value() && response->status == Status::kOk;
}

std::int32_t SocketClient::add(const std::int32_t a, const std::int32_t b) {
    Frame frame;
    frame.opcode = Opcode::kAdd;
    frame.payload.resize(8);
    std::memcpy(frame.payload.data(), &a, sizeof(a));
    std::memcpy(frame.payload.data() + 4, &b, sizeof(b));
    const auto response = transact(frame);
    if (!response || response->status != Status::kOk || response->payload.size() < 4) {
        return -1;
    }
    std::int32_t result = 0;
    std::memcpy(&result, response->payload.data(), sizeof(result));
    return result;
}

std::optional<std::vector<std::uint8_t>> SocketClient::process_custom(
    const std::span<const std::uint8_t> payload) {
    const auto response =
        transact(Frame{.opcode = Opcode::kProcessCustom,
                       .payload = std::vector<std::uint8_t>(payload.begin(), payload.end())});
    if (!response || response->status != Status::kOk) {
        return std::nullopt;
    }
    return response->payload;
}

std::optional<std::vector<std::uint8_t>> SocketClient::process_parcel(
    const std::span<const std::uint8_t> payload) {
    const auto response =
        transact(Frame{.opcode = Opcode::kProcessParcel,
                       .payload = std::vector<std::uint8_t>(payload.begin(), payload.end())});
    if (!response || response->status != Status::kOk) {
        return std::nullopt;
    }
    return response->payload;
}

SocketClient& global_socket_client() {
    static SocketClient client("socketipc_calc");
    return client;
}

}  // namespace ipc
