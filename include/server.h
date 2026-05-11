#pragma once

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <functional>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

class UniqueFd {
  public:
    UniqueFd() noexcept = default;
    explicit UniqueFd(int fd) noexcept : fd_(fd) {}

    UniqueFd(const UniqueFd &) = delete;
    UniqueFd &operator=(const UniqueFd &) = delete;

    UniqueFd(UniqueFd &&other) noexcept : fd_(std::exchange(other.fd_, -1)) {}

    UniqueFd &operator=(UniqueFd &&other) noexcept {
        reset(std::exchange(other.fd_, -1));
        return *this;
    }

    ~UniqueFd() { reset(); }

    [[nodiscard]] int get() const noexcept { return fd_; }
    [[nodiscard]] bool is_valid() const noexcept { return fd_ >= 0; }

    void reset(int new_fd = -1) noexcept {
        if (fd_ >= 0) {
            ::close(fd_);
        }
        fd_ = new_fd;
    }

  private:
    int fd_{-1};
};

class TcpConnectionBase {
  public:
    explicit TcpConnectionBase(UniqueFd fd) : fd_(std::move(fd)), is_done_(false) {}

    virtual ~TcpConnectionBase() {
        close();
        wait();
    }

    TcpConnectionBase(const TcpConnectionBase &) = delete;
    TcpConnectionBase &operator=(const TcpConnectionBase &) = delete;
    TcpConnectionBase(TcpConnectionBase &&) = delete;
    TcpConnectionBase &operator=(TcpConnectionBase &&) = delete;

    void start() { worker_thread_ = std::thread(&TcpConnectionBase::worker_loop, this); }

    void wait() {
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }

    [[nodiscard]] bool is_done() const noexcept { return is_done_.load(std::memory_order_relaxed); }

  protected:
    virtual void receive(std::string data) = 0;

    void send(std::string_view data) {
        if (!fd_.is_valid() || is_done_.load(std::memory_order_relaxed))
            return;

        int flags = 0;
#ifdef MSG_NOSIGNAL
        flags |= MSG_NOSIGNAL;
#endif

        const std::lock_guard<std::mutex> lock(send_mutex_);
        size_t total_sent = 0;
        while (total_sent < data.size()) {
            ssize_t sent =
                ::send(fd_.get(), data.data() + total_sent, data.size() - total_sent, flags);
            if (sent < 0) {
                if (errno == EINTR)
                    continue;
                close();
                break;
            }
            total_sent += sent;
        }
    }

    void close() {
        if (!is_done_.exchange(true, std::memory_order_relaxed)) {
            if (fd_.is_valid()) {
                ::shutdown(fd_.get(), SHUT_RDWR);
            }
        }
    }

  private:
    void worker_loop() {
        constexpr size_t buffer_size = 4096;
        std::vector<char> buffer(buffer_size);

        while (!is_done_.load(std::memory_order_relaxed)) {
            ssize_t bytes_read = ::recv(fd_.get(), buffer.data(), buffer.size(), 0);

            if (bytes_read > 0) {
                receive(std::string(buffer.data(), bytes_read));
            } else if (bytes_read == 0) {
                break;
            } else {
                if (errno == EINTR)
                    continue;
                break;
            }
        }

        close();
    }

    UniqueFd fd_;
    std::atomic<bool> is_done_;
    std::thread worker_thread_;
    std::mutex send_mutex_;
};

class TcpServer final {
  public:
    using ConnectionFactory = std::function<std::unique_ptr<TcpConnectionBase>(UniqueFd)>;

    explicit TcpServer(uint16_t port, ConnectionFactory factory)
        : factory_(std::move(factory)), running_(true) {
        if (!factory_) {
            throw std::invalid_argument("Connection factory cannot be null");
        }

        int sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            throw std::system_error(errno, std::generic_category(), "Failed to create socket");
        }
        server_fd_.reset(sock);

        int opt = 1;
        if (::setsockopt(server_fd_.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            throw std::system_error(errno, std::generic_category(), "Failed to set SO_REUSEADDR");
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (::bind(server_fd_.get(), reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
            throw std::system_error(errno, std::generic_category(), "Failed to bind socket");
        }

        if (::listen(server_fd_.get(), SOMAXCONN) < 0) {
            throw std::system_error(errno, std::generic_category(), "Failed to listen on socket");
        }

        acceptor_thread_ = std::thread(&TcpServer::accept_loop, this);
    }

    ~TcpServer() {
        running_ = false;

        if (server_fd_.is_valid()) {
            ::shutdown(server_fd_.get(), SHUT_RDWR);
        }

        if (acceptor_thread_.joinable()) {
            acceptor_thread_.join();
        }

        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.clear();
    }

    TcpServer(const TcpServer &) = delete;
    TcpServer &operator=(const TcpServer &) = delete;

  private:
    void accept_loop() {
        while (running_) {
            pollfd pfd{};
            pfd.fd = server_fd_.get();
            pfd.events = POLLIN;

            int ret = ::poll(&pfd, 1, 1000);

            if (ret > 0 && (pfd.revents & POLLIN)) {
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);

                int client_sock = ::accept(server_fd_.get(),
                                           reinterpret_cast<sockaddr *>(&client_addr), &client_len);

                if (client_sock >= 0) {
                    UniqueFd client_fd(client_sock);

                    try {
                        auto conn = factory_(std::move(client_fd));
                        if (conn) {
                            conn->start();
                            std::lock_guard<std::mutex> lock(connections_mutex_);
                            connections_.push_back(std::move(conn));
                        }
                    } catch (const std::exception &e) {
                    }
                }
            }

            cleanup_connections();
        }
    }

    void cleanup_connections() {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(
            std::remove_if(
                connections_.begin(), connections_.end(),
                [](const std::unique_ptr<TcpConnectionBase> &conn) {
                    if (conn->is_done()) {
                        conn->wait(); // Thread is finishing, join it before object destruction
                        return true;
                    }
                    return false;
                }),
            connections_.end());
    }

    UniqueFd server_fd_;
    ConnectionFactory factory_;
    std::atomic<bool> running_;
    std::thread acceptor_thread_;

    std::mutex connections_mutex_;
    std::vector<std::unique_ptr<TcpConnectionBase>> connections_;
};
