#ifndef PROXYSERVER_H
#define PROXYSERVER_H

#include "ThreadPool.h"

class ProxyServer {
public:
    explicit ProxyServer(int port = 8888); // here declaration => Default value

    bool start();

private:
    int port;
    int server_fd;
    ThreadPool pool;

    void handleClient(int client_fd);
};

#endif