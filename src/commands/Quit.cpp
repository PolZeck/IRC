#include "Server.hpp"

void Server::handleQuit(int fd, std::string args) {
    std::string reason = args.empty() ? "Leaving" : args;
    if (reason[0] == ':') reason = reason.substr(1);

    std::string quitMsg = ":" + _clients[fd]->getNickname() + " QUIT :Quit: " + reason + "\r\n";

    // I need to find every channel this client was in to notify others
    std::map<std::string, Channel*>::iterator it = _channels.begin();
    while (it != _channels.end()) {
        if (it->second->hasClient(_clients[fd])) {
            it->second->broadcast(quitMsg, fd);
            it->second->removeClient(_clients[fd]);
        }
        // Optimization: I could delete empty channels here too
        ++it;
    }

    // I let my main loop handle the socket closing via removeClient
    removeClient(fd);
    std::cout << "Client on FD " << fd << " quit properly." << std::endl;
}