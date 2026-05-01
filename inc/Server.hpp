#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <vector>
#include "Client.hpp"
#include "Channel.hpp"

class Server {
    private:    
        int                              _port;
        std::string                     _password;
        int                             _serverFd;
        std::map<int, Client*>          _clients;
        std::vector<struct pollfd>      _fds; // Array for poll()
        std::map<std::string, Channel*> _channels;

    public:
        Server(int port, std::string password);
        ~Server();

        void init();
        void run();
        void removeClient(int fd);
        void acceptNewClient();
        void receiveData(int fd);

        void handleJoin(int fd, std::string args);

        void processCommand(int fd, std::string command);
        void sendResponse(int fd, std::string response);
};

#endif