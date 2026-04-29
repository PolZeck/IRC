#include "Server.hpp"
#include <errno.h>

Server::Server(int port, std::string password) : _port(port), _password(password), _serverFd(-1) {}

Server::~Server() {
    for (size_t i = 0; i < _fds.size(); i++)
        close(_fds[i].fd);
}

void Server::init() {
    // 1. Create the socket (IPv4, TCP) [cite: 108]
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0) throw std::runtime_error("Failed to create socket");

    // 2. Allow socket reuse (avoids "Address already in use" errors)
    int opt = 1;
    setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. Set to non-blocking [cite: 99, 132, 134]
    fcntl(_serverFd, F_SETFL, O_NONBLOCK);

    // 4. Bind to port
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(_port);

    if (bind(_serverFd, (struct sockaddr *)&address, sizeof(address)) < 0)
        throw std::runtime_error("Bind failed");

    // 5. Start listening
    if (listen(_serverFd, 10) < 0)
        throw std::runtime_error("Listen failed");

    // 6. Add the server socket to poll 
    struct pollfd serverPollFd;
    serverPollFd.fd = _serverFd;
    serverPollFd.events = POLLIN;
    _fds.push_back(serverPollFd);
}

void Server::run() {
    while (true) { // Use your global g_stop here normally
        // Wait for events [cite: 92, 100]
        if (poll(&_fds[0], _fds.size(), -1) < 0) break;

        for (size_t i = 0; i < _fds.size(); i++) {
            if (_fds[i].revents & POLLIN) {
                if (_fds[i].fd == _serverFd)
                    acceptNewClient();
                else
                    receiveData(_fds[i].fd);
            }
        }
    }
}

void Server::acceptNewClient() {
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int fd = accept(_serverFd, (struct sockaddr *)&clientAddr, &addrLen);
    
    if (fd != -1) {
        fcntl(fd, F_SETFL, O_NONBLOCK); // Clients must also be non-blocking [cite: 132]
        struct pollfd clientPollFd;
        clientPollFd.fd = fd;
        clientPollFd.events = POLLIN;
        _fds.push_back(clientPollFd);
        std::cout << "New client connected!" << std::endl;
    }
}

void Server::receiveData(int fd) {
    char buffer[1024];
    int bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes <= 0) {
        std::cout << "Client disconnected." << std::endl;
        close(fd);
        // Logic to remove from _fds vector needed here
    } else {
        buffer[bytes] = '\0';
        std::cout << "Received: " << buffer << std::endl;
        // Parsing of commands will go here
    }
}