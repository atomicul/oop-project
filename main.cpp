#include "network.h"
#include <csignal>
#include <iostream>

#include <client_connection.h>
#include <semaphore>
#include <thread>

std::binary_semaphore barrier{0};

void on_exit(int) { barrier.release(); }

int main() {
    constexpr uint16_t PORT = 8080;
    const char *NETWORK_NAME = "digi";

    std::signal(SIGINT, on_exit);
    std::signal(SIGTERM, on_exit);

    auto network = std::make_shared<Network>(NETWORK_NAME);
    TcpServer::ConnectionFactory factory = [network](UniqueFd fd) {
        return std::make_unique<ClientConnection>(std::move(fd), network);
    };

    std::jthread server_thread{[&factory]() {
        TcpServer server(PORT, factory);
        std::cout << "Echo server successfully started.\n";
        std::cout << "Listening on port " << PORT << "...\n";

        barrier.acquire();
        std::cout << "Shutting down the server...\n";
    }};
}
