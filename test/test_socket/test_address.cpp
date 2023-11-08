#include "amot/socket/address.h"

#include <spdlog/spdlog.h>
#include <map>
#include <string>
#include <vector>

/**
 * @brief 查询所有网卡
 * @param[in] family 地址类型
 */
void test_ifaces(int family) {
	spdlog::info("test_ifaces: {}", family);

    std::multimap<std::string, std::pair<amot::Address::ptr, uint32_t>> results;
    bool v = amot::Address::GetInterfaceAddresses(results, family);
    if (!v) {
        spdlog::error("GetInterfaceAddresses fail");
        return;
    }
    for (auto &i : results) {
		spdlog::info("{} - {} - {}", i.first, i.second.first->toString(), i.second.second);
    }
    
    spdlog::info("\n");
}

/**
 * @brief 查询指定网卡
 * @param[in] iface 网卡名称
 * @param[in] family 地址类型
 */
void test_iface(const char *iface, int family) {
	spdlog::info("test_ifaces: {}, {}", iface, family);

    std::vector<std::pair<amot::Address::ptr, uint32_t>> result;
    bool v = amot::Address::GetInterfaceAddresses(result, iface, family);
    if(!v) {
        spdlog::error("GetInterfaceAddresses fail");
        return;
    }
    for(auto &i : result) {
		spdlog::info("{} - {}", i.first->toString(), i.second);
    }

    spdlog::info("\n");
}

/**
 * @brief 测试网络地址解析
 * @param[] host 网络地址描述，包括字符串形式的域名/主机名或是数字格式的IP地址，支持端口和服务名解析
 * @note 这里没有区分不同的套接字类型，所以会有重复值
 */
void test_lookup(const char *host) {
    spdlog::info("test_lookup: {}", host);

    spdlog::info("Lookup:");
    std::vector<amot::Address::ptr> results;
    bool v = amot::Address::Lookup(results, host, AF_INET);
    if(!v) {
        spdlog::error("Lookup fail");
        return;
    }
    for(auto &i : results) {
        spdlog::info(i->toString());
    }
    
    spdlog::info("LookupAny:");
    auto addr2 = amot::Address::LookupAny(host);
    spdlog::info(addr2->toString());

    spdlog::info("LookupAnyIPAddress:");
    auto addr1 = amot::Address::LookupAnyIPAddress(host);
    spdlog::info(addr1->toString());

    spdlog::info("\n");
}

void test()
{
	std::vector<amot::Address::ptr> addrs;

	spdlog::info("begin");
	bool v = amot::Address::Lookup(addrs, "www.baidu.com:3080");
	spdlog::info("end");
	if(!v)
	{
		spdlog::info("lookup fail");
		return;
	}

	for (size_t i = 0; i < addrs.size(); ++i) {
		spdlog::info("{} - {}", i, addrs[i]->toString());
	}

	auto addr = amot::Address::LookupAny("www.google.com:4080");
	if(addr)
	{
		spdlog::info(addr->toString());
	}
	else
	{
		spdlog::info("error");
	}
}

template<class T>
static uint32_t CountBytes(T value)
{
	uint32_t result = 0;
	for (; value; ++result)
	{
		value &= value - 1;
	}
	return result;
}

void test_countbyte()
{
    spdlog::info("countbyte = {}", CountBytes(4));
}

/**
 * @brief IPv6地址类测试
 */
void test_ipv6() {
    spdlog::info("test_ipv6");

    auto addr = amot::IPAddress::Create("2402:4e00:40:40::2:3b6", 10);
    if (!addr) {
        spdlog::info("IPAddress::Create error");
        return;
    }
	spdlog::info("addr: ", addr->toString());
	spdlog::info("family: ", addr->getFamily());
	spdlog::info("port: ", addr->getPort());
	spdlog::info("addr length: ", addr->getAddrLen());

	spdlog::info("broadcast addr: ", addr->broadcastAddress(64)->toString());
    spdlog::info("network addr: ", addr->networkAddress(64)->toString());
    spdlog::info("subnet mask addr: ", addr->subnetMask(64)->toString());
}

/**
 * @brief Unix套接字解析
 */
void test_unix() {
    spdlog::info("test_unix");
    
    auto addr = amot::UnixAddress("/tmp/test_unix.sock");
    spdlog::info("addr: ", addr.toString());
    spdlog::info("family: ", addr.getFamily());  
    spdlog::info("path: ", addr.getPath()); 
    spdlog::info("addr length: ", addr.getAddrLen()); 

    spdlog::info("\n");
}

int main(int argc, char const *argv[])
{
	// 获取本机所有网卡的IPv4地址和IPv6地址以及掩码长度
    test_ifaces(AF_INET);
    test_ifaces(AF_INET6);

    // 获取本机eth0网卡的IPv4地址和IPv6地址以及掩码长度
    test_iface("eth0", AF_INET);
    test_iface("eth0", AF_INET6);

    // ip域名服务解析
    test_lookup("127.0.0.1");
    test_lookup("127.0.0.1:80");
    test_lookup("127.0.0.1:http");
    test_lookup("127.0.0.1:ftp");
    test_lookup("localhost");
    test_lookup("localhost:80");
    test_lookup("www.baidu.com");
    test_lookup("www.baidu.com:80");
    test_lookup("www.baidu.com:http");
	test_unix();
    test_countbyte();
	return 0;
}
