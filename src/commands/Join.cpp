/*
 * This file implements the JOIN command to manage user entry into channels.
 */
#include "Server.hpp"
#include <sstream>

void Server::handleJoin(int fd, std::string args) {
    std::string nick = _clients[fd]->getNickname().empty() ? "*" : _clients[fd]->getNickname();

    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, ":localhost 451 " + nick + " :You have not registered\r\n");
        return;
    }

    std::stringstream ss(args);
    std::string channelName, providedKey;
    ss >> channelName >> providedKey;

    if (channelName.empty() || channelName[0] != '#') {
        sendResponse(fd, ":localhost 461 " + nick + " JOIN :Not enough parameters or invalid channel name\r\n");
        return;
    }

    bool isCreator = false;
    if (_channels.find(channelName) == _channels.end()) {
        _channels[channelName] = new Channel(channelName);
        isCreator = true;
    }

    Channel* chan = _channels[channelName];

    if (chan->getModeI() && !chan->isInvited(fd)) {
        sendResponse(fd, ":localhost 473 " + nick + " " + channelName + " :Cannot join channel (+i)\r\n");
        return;
    }

    if (!chan->getKey().empty() && chan->getKey() != providedKey) {
        sendResponse(fd, ":localhost 475 " + nick + " " + channelName + " :Cannot join channel (+k)\r\n");
        return;
    }

    if (chan->getMaxClients() > 0 && chan->getClientCount() >= chan->getMaxClients()) {
        sendResponse(fd, ":localhost 471 " + nick + " " + channelName + " :Cannot join channel (+l)\r\n");
        return;
    }

    chan->addClient(_clients[fd]);

    if (isCreator) {
        chan->addOperator(fd);
    }

    // Message standard
    std::string userMask = nick + "!" + _clients[fd]->getUsername() + "@localhost";
    std::string joinMsg = ":" + userMask + " JOIN " + channelName + "\r\n";
    broadcastToChannel(chan, joinMsg, -1);

    // Mandatory NAMES list response after joining
    std::string listMsg = "";
    const std::vector<Client*>& channelClients = chan->getClients();
    for (size_t i = 0; i < channelClients.size(); i++) {
        if (chan->isOperator(channelClients[i]->getFd())) {
            listMsg += "@";
        }
        listMsg += channelClients[i]->getNickname() + " ";
    }
    if (!listMsg.empty() && listMsg[listMsg.size() - 1] == ' ') {
        listMsg.erase(listMsg.size() - 1);
    }

    sendResponse(fd, ":localhost 353 " + nick + " = " + channelName + " :" + listMsg + "\r\n");
    sendResponse(fd, ":localhost 366 " + nick + " " + channelName + " :End of NAMES list\r\n");
}