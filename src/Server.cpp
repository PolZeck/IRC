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

void Server::initCommands() {
    _commandMap["PASS"]     = &Server::handlePass;
    _commandMap["NICK"]     = &Server::handleNick;
    _commandMap["USER"]     = &Server::handleUser;
    _commandMap["JOIN"]     = &Server::handleJoin;
    _commandMap["PRIVMSG"]  = &Server::handlePrivmsg;
    _commandMap["PART"]     = &Server::handlePart;
    _commandMap["QUIT"]     = &Server::handleQuit;
    _commandMap["KICK"]     = &Server::handleKick;
    _commandMap["TOPIC"]    = &Server::handleTopic;
    _commandMap["INVITE"]   = &Server::handleInvite;
    _commandMap["MODE"]     = &Server::handleMode;
    _commandMap["PING"]     = &Server::handlePing;
}

void Server::processCommand(int fd, std::string command) {
    if (command.empty()) return;

    size_t spacePos = command.find_first_of(" \t\r\n");
    std::string cmdName = (spacePos == std::string::npos) ? command : command.substr(0, spacePos);
    std::string args = "";

    if (spacePos != std::string::npos) {
        args = command.substr(spacePos + 1);
        
        size_t firstNotSpace = args.find_first_not_of(" \t\r\n");
        if (firstNotSpace != std::string::npos) {
            args = args.substr(firstNotSpace);
        } else {
            args = "";
        }
    }

    size_t lastValid = args.find_last_not_of(" \t\r\n");
    if (lastValid != std::string::npos) {
        args = args.substr(0, lastValid + 1);
    } else if (!args.empty()) {
        args = "";
    }

    std::cout << "Client " << fd << " sent command: [" << cmdName << "] with args: [" << args << "]" << std::endl;

    if (_commandMap.count(cmdName))
        (this->*_commandMap[cmdName])(fd, args);
    else
        sendResponse(fd, "421 " + cmdName + " :Unknown command\r\n");
}

void Server::sendResponse(int fd, std::string response) {
    if (_clients.find(fd) != _clients.end()) {
        _clients[fd]->appendWriteBuffer(response);
        
        for (size_t i = 0; i < _fds.size(); ++i) {
            if (_fds[i].fd == fd) {
                _fds[i].events |= POLLOUT;
                break;
            }
        }
    }
}

void Server::checkRegistration(int fd) {
    Client* client = _clients[fd];
    if (!client->isRegistered() && client->isPasswordOk() && 
        !client->getNickname().empty() && client->getNickname() != "User" && 
        !client->getUsername().empty()) {
        
        client->setRegistered(true);
        std::string nick = client->getNickname();
        sendResponse(fd, "001 " + nick + " :Welcome to the IRC Network, " + nick + "\r\n");
    }
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

/* ========================================================================== */
/* NETWORK ENGINE (VERSION SÉCURISÉE)                                         */
/* ========================================================================== */

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
            if (_fds[i].revents & POLLIN)
            {
                if (_fds[i].fd == _serverFd)
                {
                    acceptNewClient();
                }
                else
                {
                    if (receiveData(_fds[i].fd) == false)
                    {
                        i--;
                        continue;
                    }
                }
            }

            if (_fds[i].revents & POLLOUT)
            {
                int clientFd = _fds[i].fd;

                std::string& writeBuffer = _clients[clientFd]->getWriteBuffer();
                
                if (!writeBuffer.empty())
                {
                    ssize_t bytesSent = send(clientFd, writeBuffer.c_str(), writeBuffer.length(), 0);
                    
                    if (bytesSent > 0)
                    {
                        writeBuffer = writeBuffer.substr(bytesSent);
                    }
                    else if (bytesSent < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
                    {
                        removeClient(clientFd);
                        i--;
                        continue;
                    }
                }

                if (writeBuffer.empty())
                {
                    _fds[i].events &= ~POLLOUT;
                }
            }
        }
    }
}

void Server::handlePing(int fd, std::string args) {
    if (args.empty()) {
        sendResponse(fd, "461 PING :Not enough parameters\r\n");
        return;
    }
    sendResponse(fd, "PONG " + args + "\r\n");
}


bool Server::receiveData(int fd)
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            removeClient(fd);
        return false;
    }
    else if (bytes == 0)
    {
        removeClient(fd);
        return false;
    }

    buffer[bytes] = '\0';
    _clients[fd]->appendBuffer(buffer);

    // On récupère une COPIE du buffer actuel
    std::string clientBuffer = _clients[fd]->getBuffer(); 
    size_t pos;

    while ((pos = clientBuffer.find('\n')) != std::string::npos)
    {
        std::string command = clientBuffer.substr(0, pos);

        if (!command.empty() && command[command.size() - 1] == '\r')
            command.erase(command.size() - 1);

        // On met à jour notre copie locale du buffer pour le reste de la boucle
        clientBuffer = clientBuffer.substr(pos + 1);
        
        // On réapplique cette modification directement dans l'objet Client
        _clients[fd]->clearBuffer();
        _clients[fd]->appendBuffer(clientBuffer);

        processCommand(fd, command);

        if (_clients.find(fd) == _clients.end())
            return false;
            
        // Si le buffer du client a reçu de nouvelles données pendant processCommand,
        // on synchronise notre copie locale pour le prochain tour de boucle
        clientBuffer = _clients[fd]->getBuffer();
    }

    return true;
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

void Server::removeClient(int fd) {
    Client* client = _clients[fd];

    std::map<std::string, Channel*>::iterator it = _channels.begin();
    while (it != _channels.end()) {
        if (it->second->hasClient(client)) {
            it->second->removeClient(client);
        }
        ++it;
    }

    for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it) {
        if (it->fd == fd) {
            _fds.erase(it);
            break;
        }
    }

    _clients.erase(fd);
    delete client;
    close(fd);
    std::cout << "Client FD " << fd << " removed safely." << std::endl;
}

void Server::broadcastToChannel(Channel* chan, std::string message, int excludeFd) {
    if (!chan) return;
    const std::vector<Client*>& clients = chan->getClients();
    for (size_t i = 0; i < clients.size(); i++) {
        if (clients[i]->getFd() != excludeFd) {
            sendResponse(clients[i]->getFd(), message);
        }
    }
}