#include "Server.hpp"
#include <errno.h>

Server::Server(int port, std::string password) : _port(port), _password(password), _serverFd(-1) {}

Server::~Server() {
    for (size_t i = 0; i < _fds.size(); i++)
        close(_fds[i].fd);
}

void Server::init() {
    // 1. Create the socket (IPv4, TCP)
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0) throw std::runtime_error("Failed to create socket");

    // 2. Allow socket reuse (avoids "Address already in use" errors)
    int opt = 1;
    setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. Set to non-blocking
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
        // Wait for events
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
        fcntl(fd, F_SETFL, O_NONBLOCK);
        _clients[fd] = new Client(fd);
        struct pollfd clientPollFd;
        clientPollFd.fd = fd;
        clientPollFd.events = POLLIN;
        _fds.push_back(clientPollFd);
        std::cout << "New client connected!" << std::endl;
    }
}

void Server::removeClient(int fd) {
    // 1. Find and remove from the pollfd vector
    for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it) {
        if (it->fd == fd) {
            _fds.erase(it);
            break;
        }
    }

    // 2. Close the socket
    close(fd);

    // 3. Delete the Client object and remove from map
    if (_clients.count(fd)) {
        delete _clients[fd]; // Free memory
        _clients.erase(fd);  // Remove key from map
    }
    
    std::cout << "Client " << fd << " has been fully removed." << std::endl;
}

void Server::receiveData(int fd) {
    char buffer[1024];
    int bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes <= 0) {
        std::cout << "Client " << fd << " disconnected." << std::endl;
        removeClient(fd); // Proper cleanup
    } else {
        buffer[bytes] = '\0';
        _clients[fd]->appendBuffer(buffer);

        // Process all complete commands in the buffer
        while (_clients[fd]->hasCommand()) {
            std::string msg = _clients[fd]->getBuffer();
            size_t pos = msg.find('\n');
            std::string command = msg.substr(0, pos);
            
            // Clean the buffer for the next command
            _clients[fd]->appendBuffer(""); // This is a placeholder logic
            // In reality: store the rest of the string back in buffer
            
            std::cout << "Executing command from " << fd << ": " << command << std::endl;
            
            // NEXT STEP: parserCommand(command, fd);
            
            // Real buffer management:
            std::string remaining = msg.substr(pos + 1);
            _clients[fd]->clearBuffer();
            _clients[fd]->appendBuffer(remaining);
        }
    }
}