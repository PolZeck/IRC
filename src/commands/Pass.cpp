#include "Server.hpp"

void Server::handlePass(int fd, std::string args) {
    // I check if the client is already registered to avoid multiple PASS attempts
    if (_clients[fd]->isRegistered()) {
        sendResponse(fd, "462 :Unauthorized command (already registered)\r\n");
        return;
    }
    // I compare the provided argument with my server's password
    if (args == _password) {
        _clients[fd]->setPasswordOk(true);
    } else {
        // If it's wrong, I send the official IRC error code 464
        sendResponse(fd, "464 :Password incorrect\r\n");
    }
}