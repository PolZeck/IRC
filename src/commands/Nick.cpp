#include "Server.hpp"

void Server::handleNick(int fd, std::string args) {
    if (!_clients[fd]->isPasswordOk()) {
        sendResponse(fd, "451 :You have not registered (PASS required)\r\n");
        return;
    }

    if (!args.empty() && args[0] == ':') args = args.substr(1);
    size_t space = args.find_first_of(" \r\n");
    if (space != std::string::npos) args = args.substr(0, space);

    if (args.empty()) {
        sendResponse(fd, "431 :No nickname given\r\n");
        return;
    }

    if (findClientByNick(args) != NULL) {
        sendResponse(fd, "433 " + args + " :Nickname is already in use\r\n");
        return;
    }

    std::string oldNick = _clients[fd]->getNickname();
    _clients[fd]->setNickname(args);
    std::cout << "Client " << fd << " changed nickname to " << args << std::endl;

    if (_clients[fd]->isRegistered()) {
        std::string nickMsg = ":" + oldNick + " NICK " + args + "\r\n";
        sendResponse(fd, nickMsg);
    } else {
        checkRegistration(fd);
    }
}