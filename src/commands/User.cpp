#include "Server.hpp"

void Server::handleUser(int fd, std::string args) {
    // Standard IRC checks for registration flow
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
    
    // Once USER is received, I consider the client fully registered
    _clients[fd]->setRegistered(true);
    std::string nick = _clients[fd]->getNickname();
    sendResponse(fd, "001 " + nick + " :Welcome to the IRC Network, " + nick + "\r\n");
}