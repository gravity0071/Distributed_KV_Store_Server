#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>

class Server {
public:
    explicit Server(int port);
    Server();

    ~Server();

    // 初始化服务器
    bool initialize();
    int acceptConnection();
    void closeConnection(int client_socket);
    void closeServer();
    int getSocket() const;
    std::string getServerIP() const;
    int getServerPort() const;

private:
    int port_;                   
    int server_fd_;              
    struct sockaddr_in address_; 
    int addrlen_;                
};
