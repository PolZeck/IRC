#include "Server.hpp"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

extern bool g_stop;

Server::Server(int port, std::string password) : _port(port), _password(password), _serverFd(-1) {
    // Mapping my commands right at the start so the dispatcher is ready
    initCommands(); 
}

Server::~Server() {
    // Cleaning up my client objects to avoid memory leaks
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        delete it->second;
    
    // Cleaning up my channel objects
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
        delete it->second;

    // Closing all active sockets
    for (size_t i = 0; i < _fds.size(); i++)
        close(_fds[i].fd);
}

void Server::initCommands() {
    _commandMap["PASS"]    = &Server::handlePass;
    _commandMap["NICK"]    = &Server::handleNick;
    _commandMap["USER"]    = &Server::handleUser;
    _commandMap["JOIN"]    = &Server::handleJoin;
    _commandMap["PRIVMSG"] = &Server::handlePrivmsg;
    _commandMap["PART"]    = &Server::handlePart;
    _commandMap["QUIT"]    = &Server::handleQuit;
    _commandMap["KICK"]    = &Server::handleKick;
}

void Server::processCommand(int fd, std::string command) {
    if (command.empty()) return;

    // I split the incoming string into command name and arguments
    size_t spacePos = command.find(' ');
    std::string cmdName = command.substr(0, spacePos);
    std::string args = (spacePos != std::string::npos) ? command.substr(spacePos + 1) : "";

    std::cout << "Client " << fd << " sent: [" << cmdName << "]" << std::endl;

    // Using my map to call the right function instead of a giant if/else block
    if (_commandMap.count(cmdName)) {
        (this->*_commandMap[cmdName])(fd, args);
    } else {
        // If I haven't coded the command yet, I send an 'Unknown command' error
        sendResponse(fd, "421 " + cmdName + " :Unknown command\r\n");
    }
}

void Server::sendResponse(int fd, std::string response) {
    // Simple wrapper to send raw data to a specific socket
    send(fd, response.c_str(), response.length(), 0);
}

/* ========================================================================== */
/* NETWORK ENGINE                                                             */
/* ========================================================================== */

void Server::init() {
    // Creating my main listener socket
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0) throw std::runtime_error("Failed to create socket");

    // Setting SO_REUSEADDR so I don't have to wait for the port to clear after a crash
    int opt = 1;
    setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // I must set the server socket to non-blocking as per the subject
    fcntl(_serverFd, F_SETFL, O_NONBLOCK);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(_port);

    if (bind(_serverFd, (struct sockaddr *)&address, sizeof(address)) < 0)
        throw std::runtime_error("Bind failed");

    if (listen(_serverFd, 10) < 0)
        throw std::runtime_error("Listen failed");

    // Adding my listener to the poll list
    struct pollfd serverPollFd;
    serverPollFd.fd = _serverFd;
    serverPollFd.events = POLLIN;
    _fds.push_back(serverPollFd);
}

void Server::run() {
    // Main loop: I keep running until g_stop is true (Ctrl+C)
    while (g_stop == false) {
        // I wait for poll to tell me which sockets have data waiting
        if (poll(&_fds[0], _fds.size(), -1) < 0 && g_stop == false)
            break;

        for (size_t i = 0; i < _fds.size(); i++) {
            if (_fds[i].revents & POLLIN) {
                if (_fds[i].fd == _serverFd)
                    acceptNewClient(); // New connection arriving
                else
                    receiveData(_fds[i].fd); // Existing client sending data
            }
        }
    }
}

Client* Server::findClientByNick(std::string nick) {
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second->getNickname() == nick)
            return it->second;
    }
    return NULL;
}

void Server::acceptNewClient() {
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int fd = accept(_serverFd, (struct sockaddr *)&clientAddr, &addrLen);
    
    if (fd != -1) {
        // I make sure every new client is non-blocking too
        fcntl(fd, F_SETFL, O_NONBLOCK);
        _clients[fd] = new Client(fd); 
        
        struct pollfd clientPollFd;
        clientPollFd.fd = fd;
        clientPollFd.events = POLLIN;
        _fds.push_back(clientPollFd);
    }
}

void Server::removeClient(int fd) {
    // I remove them from my poll list first
    for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it) {
        if (it->fd == fd) {
            _fds.erase(it);
            break;
        }
    }
    close(fd);
    // I delete their data and remove them from my map
    if (_clients.count(fd)) {
        delete _clients[fd];
        _clients.erase(fd);
    }
}

void Server::receiveData(int fd) {
    char buffer[1024];
    int bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes <= 0) {
        // Client disconnected or error
        removeClient(fd);
    } else {
        buffer[bytes] = '\0';
        _clients[fd]->appendBuffer(buffer);

        // I process every complete command found in my buffer
        while (_clients[fd]->hasCommand()) {
            std::string fullCommand = _clients[fd]->getBuffer();
            size_t pos = fullCommand.find('\n');
            std::string command = fullCommand.substr(0, pos);

            // Removing \r if it exists for clean string handling
            if (!command.empty() && command[command.size() - 1] == '\r')
                command.erase(command.size() - 1);

            // Dispatching the command
            this->processCommand(fd, command);

            // Moving the rest of the data forward in my buffer
            std::string remaining = fullCommand.substr(pos + 1);
            _clients[fd]->clearBuffer();
            _clients[fd]->appendBuffer(remaining);
        }
    }
}