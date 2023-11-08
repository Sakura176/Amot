#pragma once

#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <map>

#include "endian.h"

namespace amot {
class IPAddress;

/**
 * @brief 网络地址的基类，抽象类
 */
class Address {
public:
	typedef std::shared_ptr<Address> ptr;

	/**
	 * @brief 通过sockaddr指针创建Address
	 * @param[in] addr sockaddr指针
	 * @param[in] addrlen sockaddr的长度
	 * @return 返回和sockaddr相匹配的Address,失败返回nullptr
	 */
	static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);

	/**
	 * @brief 通过host地址返回对应条件的所有Address
	 * @param[out] result 保存满足条件的Address
	 * @param[in] host 域名,服务器名等.举例: www.sylar.top[:80] (方括号为可选内容)
	 * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
	 * @param[in] type socketl类型SOCK_STREAM、SOCK_DGRAM 等
	 * @param[in] protocol 协议,IPPROTO_TCP、IPPROTO_UDP 等
	 * @return 返回是否转换成功
	 */
	static bool Lookup(std::vector<Address::ptr>& result, const std::string& host,
			int family = AF_INET, int type = 0, int protocol = 0);
	/**
	 * @brief 通过host地址返回对应条件的任意Address
	 * @param[in] host 域名,服务器名等.举例: www.sylar.top[:80] (方括号为可选内容)
	 * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
	 * @param[in] type socketl类型SOCK_STREAM、SOCK_DGRAM 等
	 * @param[in] protocol 协议,IPPROTO_TCP、IPPROTO_UDP 等
	 * @return 返回满足条件的任意Address,失败返回nullptr
	 */
	static Address::ptr LookupAny(const std::string& host,
			int family = AF_INET, int type = 0, int protocol = 0);
	/**
	 * @brief 通过host地址返回对应条件的任意IPAddress
	 * @param[in] host 域名,服务器名等.举例: www.sylar.top[:80] (方括号为可选内容)
	 * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
	 * @param[in] type socketl类型SOCK_STREAM、SOCK_DGRAM 等
	 * @param[in] protocol 协议,IPPROTO_TCP、IPPROTO_UDP 等
	 * @return 返回满足条件的任意IPAddress,失败返回nullptr
	 */
	static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
			int family = AF_INET, int type = 0, int protocol = 0);

	/**
	 * @brief 返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
	 * @param[out] result 保存本机所有地址
	 * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
	 * @return 是否获取成功
	 */
	static bool GetInterfaceAddresses(std::multimap<std::string
					,std::pair<Address::ptr, uint32_t> >& result,
					int family = AF_INET);
	/**
	 * @brief 获取指定网卡的地址和子网掩码位数
	 * @param[out] result 保存指定网卡所有地址
	 * @param[in] iface 网卡名称
	 * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
	 * @return 是否获取成功
	 */
	static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result
					,const std::string& iface, int family = AF_INET);

	/**
	 * @brief 虚析构函数
	 */
	virtual ~Address() {}

	/**
	 * @brief 返回协议簇
	 */
	int getFamily() const;

	/**
	 * @brief 返回sockaddr指针,只读
	 */
	virtual const sockaddr* getAddr() const = 0;

	/**
	 * @brief 返回sockaddr指针,读写
	 */
	virtual sockaddr* getAddr() = 0;

	/**
	 * @brief 返回sockaddr的长度
	 */
	virtual socklen_t getAddrLen() const = 0;

	/**
	 * @brief 可读性输出地址
	 */
	virtual std::ostream& insert(std::ostream& os) const = 0;

	/**
	 * @brief 返回可读性字符串
	 */
	std::string toString() const;

	/**
	 * @brief 小于号比较函数
	 */
	bool operator<(const Address& rhs) const;

	/**
	 * @brief 等于函数
	 */
	bool operator==(const Address& rhs) const;

	/**
	 * @brief 不等于函数
	 */
	bool operator!=(const Address& rhs) const;
};

/**
 * @brief IP地址的基类
 * 
 */
class IPAddress : public Address {
public:
	typedef std::shared_ptr<IPAddress> ptr;

	/**
	 * @brief 通过域名,IP,服务器名创建IPAddress
	 * @param[in] address 域名,IP,服务器名等.举例: www.sylar.top
	 * @param[in] port 端口号
	 * @return 调用成功返回IPAddress,失败返回nullptr
	 */
	static IPAddress::ptr Create(const char* address, uint16_t port = 0);

	/**
	 * @brief 获取该地址的广播地址
	 * @param[in] prefix_len 子网掩码位数
	 * @return 调用成功返回IPAddress，失败返回nullptr
	 */
	virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

	/**
	 * @brief 获取该地址的网段
	 * @param[in] prefix_len 子网掩码位数
	 * @return 调用成功返回IPAddres，失败返回nullptr
	 */
	virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;

	/**
	 * @brief 获取子网掩码地址
	 * @param[in] prefix_len 子网掩码位数
	 * @return 调用成功返回IPAddress，失败返回nullptr
	 */
	virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

	/**
	 * @brief 返回端口号
	 * 
	 */
	virtual uint32_t getPort() const = 0;

	/**
	 * @brief 设置端口号
	 * 
	 */
	virtual void setPort(uint16_t v) = 0;
};

/**
 * @brief IPv4地址
 */
class IPv4Address : public IPAddress {
public:
	typedef std::shared_ptr<IPv4Address> ptr;

	/**
	 * @brief 使用点分十进制地址创建IPv4Address
	 * @param[in] address 点分十进制地址，如：192.168.1.1
	 * @param[in] port 端口号
	 * @return 返回IPv4Address,失败返回nullptr
	 */
	static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

	/**
	 * @brief 通过sockaddr_in构造IPv4Address
	 * @param[in] address sockaddr_in结构体
	 */
	IPv4Address(const sockaddr_in& address);

	/**
	 * @brief 通过二进制地址构造IPv4Address
	 * @param[in] address 二进制address
	 * @param[in] port 端口号
	 */
	IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

	const sockaddr* getAddr() const override;
	sockaddr* getAddr() override;
	socklen_t getAddrLen() const override;
	/**
	 * @brief 地址信息转为点分进制表示
	 * 
	 * @param os 
	 * @return std::ostream& 
	 */
	std::ostream& insert(std::ostream& os) const override;

	IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
	IPAddress::ptr networkAddress(uint32_t prefix_len) override;
	IPAddress::ptr subnetMask(uint32_t prefix_len) override;
	uint32_t getPort() const override;
	void setPort(uint16_t v) override;
private:
	sockaddr_in m_addr;
};

/**
 * @brief IPV6地址
 */
class IPv6Address : public IPAddress {
public:
	typedef std::shared_ptr<IPv6Address> ptr;

	/**
	 * @brief 使用点分十六进制地址创建IPv6Address
	 * @param[in] address 冒分十六进制地址，如：2031:0000:1F1F:0000:0000:0100:11A0:ADDF
	 * @param[in] port 端口号
	 * @return 返回IPv6Address,失败返回nullptr
	 */
	static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

	/**
	 * @brief 无参构造函数
	 */
	IPv6Address();

	/**
	 * @brief 通过sockaddr_in6构造IPv6Address
	 * @param[in] address sockaddr_in6结构体
	 */
	IPv6Address(const sockaddr_in6& address);

	/**
	 * @brief 通过二进制地址构造IPv4Address
	 * @param[in] address 二进制address
	 * @param[in] port 端口号
	 */
	IPv6Address(const uint8_t address[16], uint16_t port = 0);

	const sockaddr* getAddr() const override;
	sockaddr* getAddr() override;
	socklen_t getAddrLen() const override;
	std::ostream& insert(std::ostream& os) const override;

	IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
	IPAddress::ptr networkAddress(uint32_t prefix_len) override;
	IPAddress::ptr subnetMask(uint32_t prefix_len) override;
	uint32_t getPort() const override;
	void setPort(uint16_t v) override;

private:
	sockaddr_in6 m_addr;
};

/**
 * @brief unix地址类
 */
class UnixAddress : public Address {
public:
	typedef std::shared_ptr<UnixAddress> ptr;

	/**
	 * @brief 无参构造函数，初始化sock地址结构体和地址长度
	 */
	UnixAddress();

	/**
	 * @brief 有参构造函数，根据路径构造unix地址
	 * 
	 * @param path 
	 */
	UnixAddress(const std::string& path);

	/**
	 * @brief 设置地址长度
	 * 
	 * @param v 
	 */
	void setAddrLen(uint32_t v);

	/**
	 * @brief 获取sock地址
	 * 
	 * @return sockaddr* sock地址指针
	 */
	sockaddr* getAddr() override;

	const sockaddr* getAddr() const override;

	/**
	 * @brief 获取sock地址长度
	 * 
	 * @return socklen_t 地址长度
	 */
	socklen_t getAddrLen() const override;

	std::string getPath() const;

	/**
	 * @brief 将地址转为字符串，由to_string函数调用
	 * 
	 * @param os 
	 * @return std::ostream& 
	 */
	std::ostream& insert(std::ostream& os) const override;
private:
	sockaddr_un m_addr;			// 地址结构体
	socklen_t m_length;			// 地址长度
};
} // namespace amot