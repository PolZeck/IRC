/*
 * This file handles the USER command, used to define the user's real name identity.
 */
#include "Server.hpp"

void Server::handleUser(int fd, std::string args) {
    std::string nick = _clients[fd]->getNickname().empty() ? "*" : _clients[fd]->getNickname();

    if (!_clients[fd]->isPasswordOk()) {
        sendResponse(fd, ":localhost 451 " + nick + " :You have not registered (PASS required)\r\n");
        return;
    }
    if (_clients[fd]->isRegistered()) {
        sendResponse(fd, ":localhost 462 " + nick + " :Unauthorized command (already registered)\r\n");
        return;
    }

    // Enforce RFC 2812: USER requires 4 parameters (<user> <mode> <unused> <realname>)
    size_t space1 = args.find(' ');
    size_t space2 = (space1 != std::string::npos) ? args.find(' ', space1 + 1) : std::string::npos;
    size_t space3 = (space2 != std::string::npos) ? args.find(' ', space2 + 1) : std::string::npos;

    if (space1 == std::string::npos || space2 == std::string::npos || space3 == std::string::npos) {
        sendResponse(fd, ":localhost 461 " + nick + " USER :Not enough parameters\r\n");
        return;
    }

    // Extract username and realname (everything after the 3rd space, cleaning colon)
    std::string username = args.substr(0, space1);
    std::string realname = args.substr(space3 + 1);
    if (!realname.empty() && realname[0] == ':') {
        realname = realname.substr(1);
    }

    _clients[fd]->setUsername(username);
    std::cout << "Client " << fd << " set username to: " << username << std::endl;

    // Check if the client is now fully registered
    checkRegistration(fd);
}