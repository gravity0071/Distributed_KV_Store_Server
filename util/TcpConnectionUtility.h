//
// Created by Shawn Wan on 2024/11/15.
//
#pragma once
#ifndef DISTRIBUTED_KV_STORE_MASTER_TCPCONNECTIONUTILITY_H
#define DISTRIBUTED_KV_STORE_MASTER_TCPCONNECTIONUTILITY_H
#include <string>

class TcpConnectionUtility {
public:
    // Create and establish a TCP connection to the specified IP and port, with an optional local port
    static int connectToServer(const std::string& serverIp, int serverPort, int localPort = 0);
};

#endif //DISTRIBUTED_KV_STORE_MASTER_TCPCONNECTIONUTILITY_H
