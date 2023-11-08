#include "amot/socket/socket.h"
#include "amot/common/macro.h"
#include "amot/coroutine/task.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

void test_socket_client()
{
    auto logger = spdlog::get("system");
    int ret;
    auto server_addr = amot::Address::LookupAnyIPAddress("0.0.0.0:3389");
    AMOT_ASSERT(server_addr);

    auto sock_client = amot::Socket::CreateTCPSocket();
    AMOT_ASSERT(sock_client);

    ret = sock_client->connect(server_addr);
    AMOT_ASSERT(ret);

    const char buff[] = "hello server";
    int rt = sock_client->send(buff, strlen(buff));
    if(rt <= 0) {
       logger->info("send fail rt={}", rt);
        return;
    }

    sock_client->close();
}

amot::Task<std::string, amot::AsyncExecutor> socket_get()
{
    auto logger = spdlog::get("system");
    amot::IPAddress::ptr addr = amot::Address::LookupAnyIPAddress("www.baidu.com");
    if(addr) {
        logger->info("get address: {}", addr->toString());
    } else {
        logger->info("get address fail");
        co_return "get address fail";
    }

    amot::Socket::ptr sock = amot::Socket::CreateTCP(addr);
    addr->setPort(80);
    logger->info("addr={}", addr->toString());
    if(!sock->connect(addr)) {
        logger->info("connect {} fail", addr->toString());
        co_return "connect " + addr->toString() + " fail";
    } else {
        logger->info("address {} connected", addr->toString());
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        logger->info("send fail rt={}", rt);
        co_return "send fail rt=" + std::to_string(rt);
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());
    co_return buffs;
}

amot::Task<void, amot::LooperExecutor> test_socket() {
    auto logger = spdlog::get("system");
    try {
        auto result = co_await socket_get();
        logger->info(result);
    } catch (std::exception &e) {
        logger->info(e.what());
    }
}

int main(int argc, char const *argv[]) {
    auto logger = spdlog::basic_logger_mt("system", "./logs/system.log", true);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] : %v");
    auto testSocket = test_socket();
    testSocket.then([logger]() {
        logger->info("test socket finish");
    }).catching([logger](std::exception &e) {
        logger->info("error occurred {}", e.what());
    });
    testSocket.get_result();
    return 0;
}
