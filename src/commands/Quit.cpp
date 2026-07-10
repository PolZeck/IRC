/*
 * This file handles the QUIT command, used to gracefully disconnect from the server.
 */
#include "Server.hpp"

void Server::handleQuit(int fd, std::string args) {
    std::string reason = args.empty() ? "Leaving" : args;
    if (reason[0] == ':') reason = reason.substr(1);

    // Notify all channels the user was participating in
    std::string quitMsg = ":" + _clients[fd]->getNickname() + " QUIT :Quit: " + reason + "\r\n";

    std::map<std::string, Channel*>::iterator it = _channels.begin();
    while (it != _channels.end()) {
        if (it->second->hasClient(_clients[fd])) {
            broadcastToChannel(it->second, quitMsg, fd);
            it->second->removeClient(_clients[fd]);
        }
        
        // Remove empty channels to prevent memory leaks
        if (it->second->isEmpty()) {
            delete it->second;
            _channels.erase(it++); 
        } else {
            ++it;
        }
    }

    // Final resource cleanup: remove client and close file descriptor
    removeClient(fd);
    std::cout << "Client on FD " << fd << " quit properly." << std::endl;
}