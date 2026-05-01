#include "Server.hpp"
#include <errno.h>

extern bool g_stop;

Server::Server(int port, std::string password) : _port(port), _password(password), _serverFd(-1) {}

Server::~Server() {
    for (size_t i = 0; i < _fds.size(); i++)
        close(_fds[i].fd);
}

void Server::processCommand(int fd, std::string command) {
    if (command.empty()) return;

    size_t spacePos = command.find(' ');
    std::string cmdName = command.substr(0, spacePos);
    std::string args = (spacePos != std::string::npos) ? command.substr(spacePos + 1) : "";

    std::cout << "Client " << fd << " sent command: [" << cmdName << "] with args: [" << args << "]" << std::endl;

    if (cmdName == "PASS") {
        if (args == _password) {
            _clients[fd]->setPasswordOk(true);
            std::cout << "Client " << fd << " password correct." << std::endl;
        } else {
            sendResponse(fd, "464 :Password incorrect\r\n");
        }
    }
    else if (cmdName == "USER") {
        if (!_clients[fd]->isPasswordOk()) {
            sendResponse(fd, "451 :You have not registered (PASS required)\r\n");
            return;
        }
        if (_clients[fd]->isRegistered()) {
            sendResponse(fd, "462 :Unauthorized command (already registered)\r\n");
            return;
        }
        
        if (args.empty()) {
            sendResponse(fd, "461 USER :Not enough parameters\r\n");
        } else {
            _clients[fd]->setRegistered(true);
            std::string nick = _clients[fd]->getNickname();
            
            std::string welcome = "001 " + nick + " :Welcome to the IRC Network, " + nick + "\r\n";
            sendResponse(fd, welcome);
            
            std::cout << "Client " << fd << " is now fully registered." << std::endl;
        }
    }
    else if (cmdName == "NICK") {
        if (args.empty()) {
            sendResponse(fd, "431 :No nickname given\r\n");
            return;
        }
        _clients[fd]->setNickname(args);
        std::cout << "Client " << fd << " is now known as " << args << std::endl;
    }
    else if (!_clients[fd]->isPasswordOk()) {
        sendResponse(fd, "451 :You have not registered\r\n");
    }
    else {
        sendResponse(fd, "421 :Unknown command\r\n");
    }
}

// Fonction utilitaire pour envoyer du texte au client
void Server::sendResponse(int fd, std::string response) {
    send(fd, response.c_str(), response.length(), 0);
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
    while (g_stop == false) { // g_stop is toggled by SIGINT (Ctrl+C) [cite: 81]
        // poll() blocks until an event occurs on a monitored socket.
        // It returns the number of descriptors ready for I/O. 
        if (poll(&_fds[0], _fds.size(), -1) < 0 && g_stop == false)
            throw std::runtime_error("poll() failed");

        for (size_t i = 0; i < _fds.size(); i++) {
            // Check if POLLIN event occurred (data is ready to be read)
            if (_fds[i].revents & POLLIN) {
                if (_fds[i].fd == _serverFd)
                    acceptNewClient(); // New connection on the listener socket [cite: 81, 100]
                else
                    receiveData(_fds[i].fd); // Existing client sent a message [cite: 81, 100]
            }
        }
    }
}

void Server::acceptNewClient() {
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    
    // accept() extracts the first connection request from the queue [cite: 81]
    int fd = accept(_serverFd, (struct sockaddr *)&clientAddr, &addrLen);
    
    if (fd != -1) {
        // MANDATORY: All I/O operations must be non-blocking 
        fcntl(fd, F_SETFL, O_NONBLOCK);
        
        // Store the new client in our map to keep track of its data [cite: 142]
        _clients[fd] = new Client(fd); 

        // Add the new socket to poll()'s monitoring list [cite: 100]
        struct pollfd clientPollFd;
        clientPollFd.fd = fd;
        clientPollFd.events = POLLIN;
        _fds.push_back(clientPollFd);
        
        std::cout << "New client connected on FD " << fd << std::endl;
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
        removeClient(fd);
    } else {
        buffer[bytes] = '\0';
        _clients[fd]->appendBuffer(buffer);

        while (_clients[fd]->hasCommand()) {
            // 1. On récupère la commande brute
            std::string fullCommand = _clients[fd]->getBuffer();
            size_t pos = fullCommand.find('\n');
            std::string command = fullCommand.substr(0, pos);

            // 2. On nettoie les \r (pour la compatibilité Windows/IRC)
            if (!command.empty() && command[command.size() - 1] == '\r')
                command.erase(command.size() - 1);

            // 3. ICI ON APPELLE LE CERVEAU
            this->processCommand(fd, command);

            // 4. On nettoie le buffer pour la suite
            std::string remaining = fullCommand.substr(pos + 1);
            _clients[fd]->clearBuffer();
            _clients[fd]->appendBuffer(remaining);
        }
    }
}