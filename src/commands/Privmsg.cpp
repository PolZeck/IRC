#include "Server.hpp"

void Server::handlePrivmsg(int fd, std::string args) {
    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, "451 :You have not registered\r\n");
        return;
    }

    size_t sep = args.find(':');
    if (sep == std::string::npos || args.empty()) {
        sendResponse(fd, "412 :No text to send\r\n");
        return;
    }

    std::string target = args.substr(0, sep);
    // Trimming extra spaces from the target name
    size_t lastSpace = target.find_last_not_of(" ");
    if (lastSpace != std::string::npos)
        target = target.substr(0, lastSpace + 1);

    std::string text = args.substr(sep);
    std::string sender = _clients[fd]->getNickname();
    std::string fullMsg = ":" + sender + " PRIVMSG " + target + " " + text + "\r\n";

    // Scenario 1: Sending to a channel
    if (target[0] == '#') {
        if (_channels.count(target)) {
            if (!_channels[target]->hasClient(_clients[fd])) {
                sendResponse(fd, "404 " + target + " :Cannot send to channel\r\n");
                return;
            }
            // ------------------------------------------
            _channels[target]->broadcast(fullMsg, fd);
        } else {
            sendResponse(fd, "403 " + target + " :No such channel\r\n");
    }
}
    // Scenario 2: Sending a private message to a specific user
    else {
        int targetFd = -1;
        for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
            if (it->second->getNickname() == target) {
                targetFd = it->first;
                break;
            }
        }
        if (targetFd != -1) {
            sendResponse(targetFd, fullMsg);
        } else {
            sendResponse(fd, "401 " + target + " :No such nick/channel\r\n");
        }
    }
}