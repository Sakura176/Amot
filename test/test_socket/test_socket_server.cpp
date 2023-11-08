#include "amot/socket/socket.h"
#include "amot/common/macro.h"
#include "amot/coroutine/scheduler.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>


void test_socket_server() {
    auto logger = spdlog::get("system");
    auto server_addr = amot::Address::LookupAnyIPAddress("0.0.0.0:3389");
    AMOT_ASSERT(server_addr);

    auto sock_server = amot::Socket::CreateTCPSocket();
    AMOT_ASSERT(sock_server);

    bool rt = sock_server->bind(server_addr);
    AMOT_ASSERT(rt);

    rt = sock_server->listen();
    AMOT_ASSERT(rt);

    logger->info("server listen begin ...");

    while (true) {
        auto client = sock_server->accept();
        AMOT_ASSERT(client);
        logger->info("client {} connected", client->toString());
        std::string buffer;
        buffer.resize(1024);
        int rt = client->recv(&buffer[0], buffer.size());
        if(rt) {
            logger->info("recv: {}", buffer);
        }
    }
}

int main(int argc, char const *argv[]) {
    auto logger = spdlog::basic_logger_mt("system", "./logs/system.log", true);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] : %v");
    spdlog::set_level(spdlog::level::debug);
    test_socket_server();
    return 0;
}
