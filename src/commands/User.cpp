#include "Server.hpp"

void Server::handleUser(int fd, std::string args) {
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
        return;
    }
    
    size_t space = args.find(' ');
    std::string username = (space == std::string::npos) ? args : args.substr(0, space);
    _clients[fd]->setUsername(username);

    checkRegistration(fd);
}