#pragma once

#include "epoller.h"
#include "threadPool.h"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h> // fcntl()
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h> // close()
#include <unordered_map>

namespace amot {
class WebServer {
public:
    WebServer(int port, int trigMode, int timeoutMS, bool OptLinger,
              int sqlPort, char const *sqlUser, char const *sqlPwd,
              char const *dbName, int connPoolNum, int threadNum, bool openLog,
              int logLevel, int logQueSize);

    ~WebServer();
    void Start();

private:
    bool InitSocket_();
    void InitEventMode_(int trigMode);
    void AddClient_(int fd, sockaddr_in addr);

    void DealListen_();
    void DealWrite_(HttpConn *client);
    void DealRead_(HttpConn *client);

    void SendError_(int fd, char const *info);
    void ExtentTime_(HttpConn *client);
    void CloseConn_(HttpConn *client);

    void OnRead_(HttpConn *client);
    void OnWrite_(HttpConn *client);
    void OnProcess(HttpConn *client);

    static int const MAX_FD = 65536;

    static int SetFdNonblock(int fd);

    int port_;
    bool openLinger_;
    int timeoutMS_; /* 毫秒MS */
    bool isClose_;
    int listenFd_;
    char *srcDir_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
};
} // namespace amot

