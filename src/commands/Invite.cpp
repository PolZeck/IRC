#include "Server.hpp"

void Server::handleInvite(int fd, std::string args) {
    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, "451 :You have not registered\r\n");
        return;
    }

    size_t space = args.find(' ');
    if (space == std::string::npos) {
        sendResponse(fd, "461 INVITE :Not enough parameters\r\n");
        return;
    }

    std::string targetNick = args.substr(0, space);
    std::string channelName = args.substr(space + 1);

    Client* target = findClientByNick(targetNick);
    if (!target) {
        sendResponse(fd, "401 " + targetNick + " :No such nick/channel\r\n");
        return;
    }

    if (_channels.find(channelName) == _channels.end()) {
        sendResponse(fd, "403 " + channelName + " :No such channel\r\n");
        return;
    }

    Channel* chan = _channels[channelName];

    if (!chan->hasClient(_clients[fd])) {
        sendResponse(fd, "442 " + channelName + " :You're not on that channel\r\n");
        return;
    }

    if (chan->getModeI() && !chan->isOperator(fd)) {
        sendResponse(fd, "482 " + _clients[fd]->getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return;
    }

    if (chan->hasClient(target)) {
        sendResponse(fd, "443 " + targetNick + " " + channelName + " :is already on channel\r\n");
        return;
    }

    // I record the invitation inside the channel target system
    chan->addInvite(target->getFd());

    std::string inviteMsg = ":" + _clients[fd]->getNickname() + " INVITE " + targetNick + " :" + channelName + "\r\n";
    sendResponse(target->getFd(), inviteMsg);
    sendResponse(fd, "341 " + _clients[fd]->getNickname() + " " + targetNick + " " + channelName + "\r\n");
}