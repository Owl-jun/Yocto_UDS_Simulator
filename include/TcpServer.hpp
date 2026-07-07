#pragma once

#include <cstdint>
#include <functional>
#include <string>

class TcpServer {
public:
    using RequestHandler = std::function<std::string(const std::string&)>;

    TcpServer(std::uint16_t port, RequestHandler handler);
    void run();

private:
    void handle_client(int client_fd);

    std::uint16_t port_;
    RequestHandler handler_;
};
