#pragma once

#include <atomic>
#include <string>

namespace ipc {

class SocketServer {
public:
    [[nodiscard]] bool start(const std::string& socket_name);
    void stop();
    [[nodiscard]] bool is_running() const noexcept;

private:
    void loop();

    std::string socket_name_;
    std::atomic<bool> running_{false};
    int server_fd_{-1};
};

[[nodiscard]] SocketServer& global_socket_server();

}  // namespace ipc
