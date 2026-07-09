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

    size_t space1 = args.find(' ');
    size_t space2 = (space1 != std::string::npos) ? args.find(' ', space1 + 1) : std::string::npos;
    size_t space3 = (space2 != std::string::npos) ? args.find(' ', space2 + 1) : std::string::npos;

    if (space1 == std::string::npos || space2 == std::string::npos || space3 == std::string::npos) {
        sendResponse(fd, "461 USER :Not enough parameters\r\n");
        return;
    }

    std::string username = args.substr(0, space1);
    
    std::string realname = args.substr(space3 + 1);
    
    if (!realname.empty() && realname[0] == ':') {
        realname = realname.substr(1);
    }

    _clients[fd]->setUsername(username);
    

    checkRegistration(fd);
}