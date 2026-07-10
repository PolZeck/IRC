/*
 * This file manages the PASS command for initial connection authentication.
 */
#include "Server.hpp"

void Server::handlePass(int fd, std::string args) {
    // PASS cannot be used after successful registration
    if (_clients[fd]->isRegistered()) {
        sendResponse(fd, "462 :Unauthorized command (already registered)\r\n");
        return;
    }

    // Verify provided password against server password
    if (args == _password) {
        _clients[fd]->setPasswordOk(true);
    } else {
        sendResponse(fd, "464 :Password incorrect\r\n");
    }
}