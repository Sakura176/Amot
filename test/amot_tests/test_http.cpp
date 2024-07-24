#include "amot/common/threadPool.h"
#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <netdb.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

int check_error(char const *msg, int error_no) {
    if (error_no == -1) {
        SPDLOG_ERROR("{} error[{}]", msg, strerror(errno));
        throw;
    }
    return error_no;
}

size_t check_error(char const *msg, ssize_t error_no) {
    if (error_no == -1) {
        SPDLOG_ERROR("{} error[{}]", msg, strerror(errno));
        throw;
    }
    return error_no;
}

#define CHECK_CALL(func, ...) check_error(#func, func(__VA_ARGS__))

struct socket_address_fatptr {
    struct sockaddr *m_addr;
    socklen_t m_addrlen;
};

struct socket_address_storage {
    union {
        struct sockaddr m_addr;
        struct sockaddr_storage m_addr_storage;
    };

    socklen_t m_addrlen;

    operator socket_address_fatptr() {
        return {&m_addr, m_addrlen};
    }
};

struct address_resolved_entry {
    struct addrinfo *m_curr = nullptr;

    socket_address_fatptr get_address() const {
        return {m_curr->ai_addr, m_curr->ai_addrlen};
    }

    int create_socket() const {
        int sockfd = CHECK_CALL(socket, m_curr->ai_family, m_curr->ai_socktype,
                                m_curr->ai_protocol);
        return sockfd;
    }

    int create_socket_and_bind() {
        int sockfd = create_socket();
        socket_address_fatptr addr = get_address();
        // 绑定端口
        CHECK_CALL(bind, sockfd, addr.m_addr, addr.m_addrlen);
        return sockfd;
    }

    [[nodiscard]] bool next_entry() {
        m_curr = m_curr->ai_next;
        if (m_curr == nullptr) {
            return false;
        }
        return true;
    }
};

struct address_resolver {
    struct addrinfo *m_head = nullptr;

    address_resolved_entry resolve(std::string const &name,
                                   std::string const &service) {
        int err = getaddrinfo(name.c_str(), service.c_str(), NULL, &m_head);
        if (err != 0) {
            SPDLOG_ERROR("addrinfo error: {} {}", gai_strerror(err), err);
            throw;
        }
        return {m_head};
    }

    address_resolved_entry get_first_entry() {
        return {m_head};
    }

    address_resolver() = default;

    address_resolver(address_resolver &&that) : m_head(that.m_head) {
        that.m_head = nullptr;
    }

    ~address_resolver() {
        if (m_head) {
            freeaddrinfo(m_head);
        }
    }
};

int main() {
    spdlog::set_pattern(
        "%^[%Y-%m-%d %H:%M:%S.%e] [%s:%#] [%l] [thread %t]%$: %v");
    int port = 8080;
    std::string address = "0.0.0.0";
    SPDLOG_INFO("正在监听 {} ...", address + ":" + std::to_string(port));
    address_resolver resolver;
    auto entry = resolver.resolve(address, std::to_string(port));
    int listenfd = entry.create_socket_and_bind();

    // 监听端口
    CHECK_CALL(listen, listenfd, SOMAXCONN);
    while (true) {
        // 接受连接
        socket_address_storage addr;
        int connid =
            CHECK_CALL(accept, listenfd, &addr.m_addr, &addr.m_addrlen);
        amot::ThreadPool tp;
        tp.scheduleById([connid] {
            char buf[1024];
            size_t ret = CHECK_CALL(read, connid, buf, sizeof(buf));
            std::string response = "HTTP/1.1 200 OK\n\nHello, World!";
            CHECK_CALL(write, connid, response.c_str(), response.size());
            close(connid);
        });
    }
    return 0;
}
