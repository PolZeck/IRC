#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

class Server {
private:
    int         _port;
    std::string _password;
    int         _serverFd;

public:
    Server(int port, std::string password);
    ~Server();

    void init(); // Initialisation (socket, bind, listen)
    void run();  // Boucle principale (poll)
};

#endif