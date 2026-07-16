/*
 * This file handles the INVITE command, allowing channel operators to 
 * invite users to join an invite-only (+i) channel.
 */
#include "Server.hpp"

void Server::handleInvite(int fd, std::string args) {
    std::string nick = _clients[fd]->getNickname().empty() ? "*" : _clients[fd]->getNickname();

    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, ":localhost 451 " + nick + " :You have not registered\r\n");
        return;
    }

    size_t space = args.find(' ');
    if (space == std::string::npos) {
        sendResponse(fd, ":localhost 461 " + nick + " INVITE :Not enough parameters\r\n");
        return;
    }

    std::string targetNick = args.substr(0, space);
    std::string channelName = args.substr(space + 1);

    Client* target = findClientByNick(targetNick);
    if (!target) {
        sendResponse(fd, ":localhost 401 " + nick + " " + targetNick + " :No such nick/channel\r\n");
        return;
    }

    if (_channels.find(channelName) == _channels.end()) {
        sendResponse(fd, ":localhost 403 " + nick + " " + channelName + " :No such channel\r\n");
        return;
    }

    Channel* chan = _channels[channelName];

    if (!chan->hasClient(_clients[fd])) {
        sendResponse(fd, ":localhost 442 " + nick + " " + channelName + " :You're not on that channel\r\n");
        return;
    }

    if (chan->getModeI() && !chan->isOperator(fd)) {
        sendResponse(fd, ":localhost 482 " + nick + " " + channelName + " :You're not channel operator\r\n");
        return;
    }

    if (chan->hasClient(target)) {
        sendResponse(fd, ":localhost 443 " + nick + " " + targetNick + " " + channelName + " :is already on channel\r\n");
        return;
    }

    chan->addInvite(target->getFd());

    //Notification and confirmation messages
    std::string userMask = nick + "!" + _clients[fd]->getUsername() + "@localhost";
    std::string inviteMsg = ":" + userMask + " INVITE " + targetNick + " :" + channelName + "\r\n";
    sendResponse(target->getFd(), inviteMsg);
    sendResponse(fd, ":localhost 341 " + nick + " " + targetNick + " " + channelName + "\r\n");
}