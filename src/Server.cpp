#include "Server.hpp"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

extern bool g_stop;

Server::Server(int port, std::string password)
    : _port(port), _password(password), _serverFd(-1)
{
    initCommands();
}

Server::~Server()
{
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        delete it->second;

    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
        delete it->second;

    for (size_t i = 0; i < _fds.size(); i++)
        close(_fds[i].fd);
}

void Server::initCommands()
{
    _commandMap["PASS"]    = &Server::handlePass;
    _commandMap["NICK"]    = &Server::handleNick;
    _commandMap["USER"]    = &Server::handleUser;
    _commandMap["JOIN"]    = &Server::handleJoin;
    _commandMap["PRIVMSG"] = &Server::handlePrivmsg;
    _commandMap["PART"]    = &Server::handlePart;
    _commandMap["QUIT"]    = &Server::handleQuit;
    _commandMap["KICK"]    = &Server::handleKick;
}

void Server::processCommand(int fd, std::string command)
{
    if (command.empty())
        return;

    size_t spacePos = command.find(' ');
    std::string cmdName = command.substr(0, spacePos);
    std::string args;

    if (spacePos != std::string::npos)
        args = command.substr(spacePos + 1);
    else
        args = "";

    std::cout << "Client " << fd << " sent: [" << cmdName << "]" << std::endl;

    if (_commandMap.count(cmdName))
        (this->*_commandMap[cmdName])(fd, args);
    else
        sendResponse(fd, "421 " + cmdName + " :Unknown command\r\n");
}

void Server::sendResponse(int fd, std::string response)
{
    send(fd, response.c_str(), response.length(), 0);
}

/* ========================================================================== */
/* NETWORK ENGINE                                                             */
/* ========================================================================== */

void Server::init()
{
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0)
        throw std::runtime_error("Failed to create socket");

    int opt = 1;
    setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    fcntl(_serverFd, F_SETFL, O_NONBLOCK);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(_port);

    if (bind(_serverFd, (struct sockaddr *)&address, sizeof(address)) < 0)
        throw std::runtime_error("Bind failed");

    if (listen(_serverFd, 10) < 0)
        throw std::runtime_error("Listen failed");

    struct pollfd serverPollFd;
    serverPollFd.fd = _serverFd;
    serverPollFd.events = POLLIN;
    serverPollFd.revents = 0;

    _fds.push_back(serverPollFd);
}

void Server::run()
{
    while (g_stop == false)
    {
        int pollResult = poll(&_fds[0], _fds.size(), -1);

        if (pollResult < 0)
        {
            if (g_stop == false)
                break;
            continue;
        }

        for (size_t i = 0; i < _fds.size(); i++)
        {
            int currentFd = _fds[i].fd;
            short currentRevents = _fds[i].revents;

            if (currentRevents & POLLIN)
            {
                if (currentFd == _serverFd)
                    acceptNewClient();
                else
                {
                    receiveData(currentFd);
                    if (i > 0 && i < _fds.size() && _fds[i].fd != currentFd)
                        i--;
                }
            }
        }
    }
}

Client* Server::findClientByNick(std::string nick)
{
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->second->getNickname() == nick)
            return it->second;
    }
    return NULL;
}

void Server::acceptNewClient()
{
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int fd = accept(_serverFd, (struct sockaddr *)&clientAddr, &addrLen);

    if (fd != -1)
    {
        fcntl(fd, F_SETFL, O_NONBLOCK);

        _clients[fd] = new Client(fd);

        struct pollfd clientPollFd;
        clientPollFd.fd = fd;
        clientPollFd.events = POLLIN;
        clientPollFd.revents = 0;

        _fds.push_back(clientPollFd);
    }
}

void Server::removeClient(int fd)
{
    for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it)
    {
        if (it->fd == fd)
        {
            _fds.erase(it);
            break;
        }
    }

    close(fd);

    if (_clients.count(fd))
    {
        delete _clients[fd];
        _clients.erase(fd);
    }
}

void Server::receiveData(int fd)
{
    char buffer[1024];
    int bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes <= 0)
    {
        removeClient(fd);
        return;
    }

    buffer[bytes] = '\0';
    _clients[fd]->appendBuffer(buffer);

    while (_clients.count(fd) && _clients[fd]->hasCommand())
    {
        std::string fullCommand = _clients[fd]->getBuffer();
        size_t pos = fullCommand.find('\n');
        std::string command = fullCommand.substr(0, pos);

        if (!command.empty() && command[command.size() - 1] == '\r')
            command.erase(command.size() - 1);

        processCommand(fd, command);

        if (!_clients.count(fd))
            return;

        std::string remaining = fullCommand.substr(pos + 1);
        _clients[fd]->clearBuffer();
        _clients[fd]->appendBuffer(remaining);
    }
}