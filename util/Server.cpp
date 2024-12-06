#include "Server.h"

Server::Server(int port) : port_(port), server_fd_(-1), addrlen_(sizeof(address_)) {
    address_.sin_family = AF_INET;
    address_.sin_addr.s_addr = INADDR_ANY;
    address_.sin_port = htons(port_);
}

Server::Server() : port_(0), server_fd_(-1), addrlen_(sizeof(address_)) {
    address_.sin_family = AF_INET;
    address_.sin_addr.s_addr = INADDR_ANY;
    address_.sin_port = 0; // Let the system assign an available port
}

Server::~Server() {
    closeServer();
}

bool Server::initialize() {
    if ((server_fd_ = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        return false;
    }
//    std::cout << "Server is listening on port " << port_ << std::endl;


    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR failed");
        return false;
    }

#ifdef SO_REUSEPORT
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEPORT failed");
        return false;
    }
#endif

    if (bind(server_fd_, (struct sockaddr*)&address_, sizeof(address_)) < 0) {
        perror("Bind failed");
        return false;
    }

    // Update the assigned port if it was dynamically allocated
    if (port_ == 0) {
        sockaddr_in boundAddress{};
        socklen_t boundAddressLen = sizeof(boundAddress);
        if (getsockname(server_fd_, (struct sockaddr*)&boundAddress, &boundAddressLen) == 0) {
            port_ = ntohs(boundAddress.sin_port);
        } else {
            perror("getsockname failed");
            return false;
        }
    }

    if (listen(server_fd_, 3) < 0) {
        perror("Listen failed");
        return false;
    }
    return true;
}

int Server::acceptConnection() {
    sockaddr_in clientAddress{};
    socklen_t clientAddrLen = sizeof(clientAddress);
    int new_socket = accept(server_fd_, (struct sockaddr*)&clientAddress, &clientAddrLen);
    if (new_socket < 0) {
        perror("Accept failed");
        return -1;
    }
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddress.sin_addr, clientIP, sizeof(clientIP));
    int clientPort = ntohs(clientAddress.sin_port);
//    std::cout << "Accepted connection from " << clientIP << ":" << clientPort << std::endl;
    return new_socket;
}

void Server::closeConnection(int client_socket) {
    close(client_socket);
//    std::cout << "Closed connection with client on port " << port_ << std::endl;
}

void Server::closeServer() {
    if (server_fd_ != -1) {
        close(server_fd_);
        server_fd_ = -1;
//        std::cout << "Closed server on port " << port_ << std::endl;
    }
}

int Server::getSocket() const {
    return server_fd_;
}

std::string Server::getServerIP() const {
    char ipBuffer[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &address_.sin_addr, ipBuffer, sizeof(ipBuffer)) != nullptr) {
        return std::string(ipBuffer);
    }
    return "Unknown";
}

int Server::getServerPort() const {
    return port_;
}
