/*
 * This file handles the NICK command for setting or changing a user's nickname.
 */
#include "Server.hpp"

void Server::handleNick(int fd, std::string args) {
    std::string oldNick = _clients[fd]->getNickname();
    std::string currentNick = oldNick.empty() ? "*" : oldNick;

    if (!_clients[fd]->isPasswordOk()) {
        sendResponse(fd, ":localhost 451 " + currentNick + " :You have not registered (PASS required)\r\n");
        return;
    }

    if (!args.empty() && args[0] == ':') args = args.substr(1);
    size_t space = args.find_first_of(" \r\n");
    if (space != std::string::npos) args = args.substr(0, space);

    if (args.empty()) {
        sendResponse(fd, ":localhost 431 " + currentNick + " :No nickname given\r\n");
        return;
    }

    if (findClientByNick(args) != NULL) {
        sendResponse(fd, ":localhost 433 " + currentNick + " " + args + " :Nickname is already in use\r\n");
        return;
    }

    _clients[fd]->setNickname(args);
    std::cout << "Client " << fd << " changed nickname to " << args << std::endl;

    if (_clients[fd]->isRegistered()) {
        std::string userMask = oldNick + "!" + _clients[fd]->getUsername() + "@localhost";
        std::string nickMsg = ":" + userMask + " NICK " + args + "\r\n";
        sendResponse(fd, nickMsg);
    } else {
        checkRegistration(fd);
    }
}