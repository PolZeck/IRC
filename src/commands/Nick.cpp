#include "Server.hpp"

void Server::handleNick(int fd, std::string args) {
    // I make sure they sent a valid password first
    if (!_clients[fd]->isPasswordOk()) {
        sendResponse(fd, "451 :You have not registered (PASS required)\r\n");
        return;
    }
    if (args.empty()) {
        sendResponse(fd, "431 :No nickname given\r\n");
        return;
    }
    // I update my client object's nickname
    _clients[fd]->setNickname(args);
    std::cout << "Client " << fd << " changed nickname to " << args << std::endl;
}