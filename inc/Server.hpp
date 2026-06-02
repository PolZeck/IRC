#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <string.h>
#include <map>        // I need this for my CommandMap!
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include "Client.hpp"
#include "Channel.hpp"

class Server {
    private:
        int                         _port;
        std::string                 _password;
        int                         _serverFd;
        std::vector<struct pollfd>  _fds;        // To keep track of all my sockets for poll()
        std::map<int, Client*>      _clients;    // My list of connected clients mapped by FD
        std::map<std::string, Channel*> _channels; // My active channels

        // I define a type for my member function pointers to make the code cleaner
        typedef void (Server::*CommandHandler)(int, std::string);
        std::map<std::string, CommandHandler> _commandMap;

        // Internal setup and command logic
        void initCommands();
        void handleNick(int fd, std::string args);
        void handleJoin(int fd, std::string args);
        void handlePrivmsg(int fd, std::string args);
        void handlePass(int fd, std::string args);
        void handleUser(int fd, std::string args);
        void handlePart(int fd, std::string args);
        void handleQuit(int fd, std::string args);
        void handleKick(int fd, std::string args);
        void handleTopic(int fd, std::string args);
        void handleInvite(int fd, std::string args);
        void handleMode(int fd, std::string args);
        Client* findClientByNick(std::string nick);
    public:
        Server(int port, std::string password);
        ~Server();

        // My core engine functions
        void init();
        void run();
        
        // My socket management
        void acceptNewClient();
        bool receiveData(int fd);
        void removeClient(int fd);
        void handlePing(int fd, std::string args);

        // Communication and parsing
        void processCommand(int fd, std::string command);
        void sendResponse(int fd, std::string response);
};

#endif