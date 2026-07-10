/*
 * This file implements the JOIN command to manage user entry into channels.
 */
#include "Server.hpp"
#include <sstream>

void Server::handleJoin(int fd, std::string args) {
    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, "451 :You have not registered\r\n");
        return;
    }

    std::stringstream ss(args);
    std::string channelName, providedKey;
    ss >> channelName >> providedKey;

    // Standard RFC: Channels must start with '#' or '&'
    if (channelName.empty() || channelName[0] != '#') {
        sendResponse(fd, "461 JOIN :Not enough parameters or invalid channel name\r\n");
        return;
    }

    // Create channel if it doesn't exist; the joiner becomes the first operator
    bool isCreator = false;
    if (_channels.find(channelName) == _channels.end()) {
        _channels[channelName] = new Channel(channelName);
        isCreator = true;
    }

    Channel* chan = _channels[channelName];

    // Mode +i: Check if the user is in the invite-list
    if (chan->getModeI() && !chan->isInvited(fd)) {
        sendResponse(fd, "473 " + _clients[fd]->getNickname() + " " + channelName + " :Cannot join channel (+i)\r\n");
        return;
    }

    // Mode +k: Check if the provided key matches the channel password
    if (!chan->getKey().empty() && chan->getKey() != providedKey) {
        sendResponse(fd, "475 " + _clients[fd]->getNickname() + " " + channelName + " :Cannot join channel (+k)\r\n");
        return;
    }

    // Mode +l: Check if channel capacity has been reached
    if (chan->getMaxClients() > 0 && chan->getClientCount() >= chan->getMaxClients()) {
        sendResponse(fd, "471 " + _clients[fd]->getNickname() + " " + channelName + " :Cannot join channel (+l)\r\n");
        return;
    }

    chan->addClient(_clients[fd]);

    // Give operator rights to the creator
    if (isCreator) {
        chan->addOperator(fd);
    }

    // Notify all members of the channel about the new arrival
    std::string joinMsg = ":" + _clients[fd]->getNickname() + " JOIN " + channelName + "\r\n";
    broadcastToChannel(chan, joinMsg, -1);
}