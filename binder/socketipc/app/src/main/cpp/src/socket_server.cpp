#include "ipc/socket_server.hpp"

#include "ipc/custom_codec.hpp"
#include "ipc/parcel_codec.hpp"
#include "ipc/socket_protocol.hpp"

#include <android/log.h>

#include <cstddef>
#include <cstring>
#include <thread>
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

[[nodiscard]] Response handle_frame(const Frame& frame) {
    LOGI("[Flow:SocketServer] handle_frame opcode=%d, payload_size=%zu, pid=%d",
         static_cast<int>(frame.opcode), frame.payload.size(), getpid());
    Response response;
    try {
        switch (frame.opcode) {
            case Opcode::kAdd: {
                if (frame.payload.size() < 8) {
                    throw std::runtime_error("add payload too short");
                }
                std::int32_t a = 0;
                std::int32_t b = 0;
                std::memcpy(&a, frame.payload.data(), sizeof(a));
                std::memcpy(&b, frame.payload.data() + 4, sizeof(b));
                const std::int32_t sum = a + b;
                response.status = Status::kOk;
                response.payload.resize(4);
                std::memcpy(response.payload.data(), &sum, sizeof(sum));
                LOGI("Socket add(%d, %d) = %d in pid=%d", a, b, sum, getpid());
                break;
            }
            case Opcode::kProcessCustom:
                LOGI("[Flow:SocketServer] kProcessCustom");
                response.status = Status::kOk;
                response.payload = CustomCodec::process(frame.payload);
                break;
            case Opcode::kProcessParcel:
                LOGI("[Flow:SocketServer] kProcessParcel");
                response.status = Status::kOk;
                response.payload = ParcelCodec::process(frame.payload);
                break;
            case Opcode::kPing:
                LOGI("[Flow:SocketServer] kPing");
                response.status = Status::kOk;
                response.payload = {'P', 'O', 'N', 'G'};
                break;
            default:
                throw std::runtime_error("unknown opcode");
        }
    } catch (const std::exception& ex) {
        LOGE("Handle frame failed: %s", ex.what());
        response.status = Status::kError;
        response.payload.assign(reinterpret_cast<const std::uint8_t*>(ex.what()),
                                reinterpret_cast<const std::uint8_t*>(ex.what()) +
                                    std::strlen(ex.what()));
    }
    return response;
}

void handle_client(const int client_fd) {
    LOGI("[Flow:SocketServer] handle_client fd=%d, pid=%d", client_fd, getpid());
    try {
        const auto request_buffer = SocketProtocol::read_fully(client_fd);
        const auto request = SocketProtocol::decode_request(request_buffer);
        const auto response = handle_frame(request);
        const auto encoded = SocketProtocol::encode_response(response);
        (void)SocketProtocol::write_fully(client_fd, encoded);
        LOGI("[Flow:SocketServer] handle_client 完成 fd=%d", client_fd);
    } catch (const std::exception& ex) {
        LOGE("Client handling failed: %s", ex.what());
    }
    close(client_fd);
}

}  // namespace

bool SocketServer::start(const std::string& socket_name) {
    LOGI("[Flow:SocketServer] start socket=%s, pid=%d", socket_name.c_str(), getpid());
    if (running_.load()) {
        return true;
    }
    socket_name_ = socket_name;
    std::thread([this]() { loop(); }).detach();
    for (int i = 0; i < 200; ++i) {
        if (running_.load()) {
            return true;
        }
        usleep(10'000);
    }
    return false;
}

void SocketServer::stop() {
    LOGI("[Flow:SocketServer] stop, pid=%d", getpid());
    running_.store(false);
    if (server_fd_ >= 0) {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }
}

bool SocketServer::is_running() const noexcept { return running_.load(); }

void SocketServer::loop() {
    server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        LOGE("Failed to create server socket");
        return;
    }

    sockaddr_un addr {};
    socklen_t addr_len = 0;
    if (!setup_abstract_address(socket_name_, &addr, &addr_len)) {
        close(server_fd_);
        server_fd_ = -1;
        return;
    }

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), addr_len) < 0) {
        LOGE("Failed to bind abstract socket %s", socket_name_.c_str());
        close(server_fd_);
        server_fd_ = -1;
        return;
    }

    if (listen(server_fd_, 64) < 0) {
        LOGE("Failed to listen on socket");
        close(server_fd_);
        server_fd_ = -1;
        return;
    }

    running_.store(true);
    LOGI("Socket server started on %s, pid=%d", socket_name_.c_str(), getpid());

    while (running_.load()) {
        const int client_fd = accept(server_fd_, nullptr, nullptr);
        if (client_fd < 0) {
            if (!running_.load()) {
                break;
            }
            continue;
        }
        LOGI("[Flow:SocketServer] accept client_fd=%d", client_fd);
        std::thread(handle_client, client_fd).detach();
    }

    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }
    running_.store(false);
    LOGI("Socket server stopped");
}

SocketServer& global_socket_server() {
    static SocketServer server;
    return server;
}

}  // namespace ipc
