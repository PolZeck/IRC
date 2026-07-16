/*
 * This file manages the PASS command for initial connection authentication.
 */
#include "Server.hpp"

void Server::handlePass(int fd, std::string args) {
    std::string nick = _clients[fd]->getNickname().empty() ? "*" : _clients[fd]->getNickname();

    // PASS cannot be used after successful registration
    if (_clients[fd]->isRegistered()) {
        sendResponse(fd, ":localhost 462 " + nick + " :Unauthorized command (already registered)\r\n");
        return;
    }

    // Verify provided password against server password
    if (args == _password) {
        _clients[fd]->setPasswordOk(true);
    } else {
        sendResponse(fd, ":localhost 464 " + nick + " :Password incorrect\r\n");
    }
}