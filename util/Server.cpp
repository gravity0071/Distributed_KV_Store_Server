#include "Server.h"

Server::Server(int port) : port_(port), server_fd_(-1), addrlen_(sizeof(address_)) {
    address_.sin_family = AF_INET;
    address_.sin_addr.s_addr = INADDR_ANY;
    address_.sin_port = htons(port_);
}

Server::~Server() {
    closeServer();
}

bool Server::initialize() {
    if ((server_fd_ = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        return false;
    }

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

    if (bind(server_fd_, (struct sockaddr *)&address_, sizeof(address_)) < 0) {
        perror("Bind failed");
        return false;
    }

    if (listen(server_fd_, 3) < 0) {
        perror("Listen failed");
        return false;
    }

//    std::cout << "Server is listening on port " << port_ << std::endl;
    return true;
}

int Server::acceptConnection() {
    int new_socket;
    if ((new_socket = accept(server_fd_, (struct sockaddr *)&address_, (socklen_t *)&addrlen_)) < 0) {
        perror("Accept failed");
        return -1;
    }
    std::cout << "Accepted new connection on port " << port_ << std::endl;
    return new_socket;
}

void Server::closeConnection(int client_socket) {
    close(client_socket);
    std::cout << "Closed connection with client on port " << port_ << std::endl;
}

void Server::closeServer() {
    if (server_fd_ != -1) {
        close(server_fd_);
        server_fd_ = -1;
        std::cout << "Closed server on port " << port_ << std::endl;
    }
}

// Add getSocket method
int Server::getSocket() const {
    return server_fd_;
}