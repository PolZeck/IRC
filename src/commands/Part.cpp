#include "Server.hpp"

void Server::handlePart(int fd, std::string args) {
    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, "451 :You have not registered\r\n");
        return;
    }

    if (args.empty()) {
        sendResponse(fd, "461 PART :Not enough parameters\r\n");
        return;
    }

    size_t space = args.find(' ');
    std::string channelName = (space == std::string::npos) ? args : args.substr(0, space);
    std::string reason = (space == std::string::npos) ? "" : args.substr(space + 1);
    
    if (!reason.empty() && reason[0] == ':') {
        reason = reason.substr(1);
    }

    if (_channels.find(channelName) == _channels.end()) {
        sendResponse(fd, "403 " + channelName + " :No such channel\r\n");
        return;
    }

    std::string partMsg = ":" + _clients[fd]->getNickname() + " PART " + channelName;
    if (!reason.empty()) {
        partMsg += " :" + reason;
    }
    partMsg += "\r\n";

    broadcastToChannel(_channels[channelName], partMsg, -1);
    _channels[channelName]->removeClient(_clients[fd]);

    if (_channels[channelName]->isEmpty()) {
        delete _channels[channelName];
        _channels.erase(channelName);
    }
}