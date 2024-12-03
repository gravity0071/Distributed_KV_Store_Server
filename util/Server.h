#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

class Server {
public:
    // 构造函数
    Server(int port);

    // 析构函数
    ~Server();

    // 初始化服务器
    bool initialize();

    // 接受新连接
    int acceptConnection();

    // 关闭与客户端的连接
    void closeConnection(int client_socket);

    // 关闭服务器
    void closeServer();

    // 获取服务器套接字（添加的 getSocket 方法）
    int getSocket() const;

private:
    int port_;                   // 端口号
    int server_fd_;              // 服务器文件描述符
    struct sockaddr_in address_; // 服务器地址
    int addrlen_;                // 地址长度
};
