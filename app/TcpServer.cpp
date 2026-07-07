#include "TcpServer.hpp"

#include "Logger.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

TcpServer::TcpServer(std::uint16_t port, RequestHandler handler)
    : port_(port)
    , handler_(std::move(handler))
{
}

void TcpServer::run()
{
    const int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::runtime_error("socket() failed");
    }

    int reuse_address = 1;
    ::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(reuse_address));

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (::bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        const std::string message = "bind() failed: " + std::string(std::strerror(errno));
        ::close(server_fd);
        throw std::runtime_error(message);
    }

    if (::listen(server_fd, 8) < 0) {
        ::close(server_fd);
        throw std::runtime_error("listen() failed");
    }

    Logger::info("TCP server listening on port " + std::to_string(port_));

    while (true) {
        sockaddr_in client_address {};
        socklen_t client_length = sizeof(client_address);
        const int client_fd = ::accept(server_fd, reinterpret_cast<sockaddr*>(&client_address), &client_length);
        if (client_fd < 0) {
            Logger::error("accept() failed");
            continue;
        }

        Logger::info("Client connected");
        handle_client(client_fd);
        ::close(client_fd);
        Logger::info("Client disconnected");
    }
}

void TcpServer::handle_client(int client_fd)
{
    std::string buffer;
    char chunk[256] {};

    while (true) {
        const ssize_t received = ::recv(client_fd, chunk, sizeof(chunk), 0);
        if (received <= 0) {
            return;
        }

        buffer.append(chunk, static_cast<std::size_t>(received));

        std::size_t newline = std::string::npos;
        while ((newline = buffer.find('\n')) != std::string::npos) {
            const std::string request = buffer.substr(0, newline);
            buffer.erase(0, newline + 1);

            const std::string response = handler_(request) + "\n";
            ::send(client_fd, response.data(), response.size(), 0);
        }
    }
}
