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

    std::string channelName = args;
    // I check if the channel exists
    if (_channels.find(channelName) == _channels.end()) {
        sendResponse(fd, "403 " + channelName + " :No such channel\r\n");
        return;
    }

    // I notify everyone that the user is leaving
    std::string partMsg = ":" + _clients[fd]->getNickname() + " PART " + channelName + "\r\n";
    _channels[channelName]->broadcast(partMsg);

    // I remove the client from the channel's internal list
    _channels[channelName]->removeClient(_clients[fd]);

    // If the channel is now empty, I delete it to save memory
    // if (_channels[channelName]->isEmpty()) {
    //     delete _channels[channelName];
    //     _channels.erase(channelName);
    // }
}