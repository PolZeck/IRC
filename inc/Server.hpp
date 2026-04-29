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

class Server {
    private:
        int                         _port;
        std::string                 _password;
        int                         _serverFd;
        std::vector<struct pollfd>  _fds; // Array for poll() 

    public:
        Server(int port, std::string password);
        ~Server();

        void init();
        void run();
        void acceptNewClient();
        void receiveData(int fd);
};

#endif