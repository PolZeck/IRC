#include "Server.hpp"

void Server::handlePass(int fd, std::string args) {
    if (_clients[fd]->isRegistered()) {
        sendResponse(fd, "462 :Unauthorized command (already registered)\r\n");
        return;
    }
    if (args == _password) {
        _clients[fd]->setPasswordOk(true);
    } else {
        sendResponse(fd, "464 :Password incorrect\r\n");
    }
}