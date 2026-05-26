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

    if (channelName.empty() || channelName[0] != '#') {
        sendResponse(fd, "461 JOIN :Not enough parameters or invalid channel name\r\n");
        return;
    }

    // Dynamic creation if channel doesn't exist
    if (_channels.find(channelName) == _channels.end()) {
        _channels[channelName] = new Channel(channelName);
    }

    Channel* chan = _channels[channelName];

    // 1. Check Invite-only Mode (+i)
    if (chan->getModeI() && !chan->isInvited(fd)) {
        sendResponse(fd, "473 " + _clients[fd]->getNickname() + " " + channelName + " :Cannot join channel (+i)\r\n");
        return;
    }

    // 2. Check Key Mode (+k)
    if (!chan->getKey().empty() && chan->getKey() != providedKey) {
        sendResponse(fd, "475 " + _clients[fd]->getNickname() + " " + channelName + " :Cannot join channel (+k)\r\n");
        return;
    }

    // 3. Check Limit Mode (+l)
    if (chan->getMaxClients() > 0 && chan->getClientCount() >= chan->getMaxClients()) {
        sendResponse(fd, "471 " + _clients[fd]->getNickname() + " " + channelName + " :Cannot join channel (+l)\r\n");
        return;
    }

    chan->addClient(_clients[fd]);

    std::string joinMsg = ":" + _clients[fd]->getNickname() + " JOIN " + channelName + "\r\n";
    chan->broadcast(joinMsg);
}