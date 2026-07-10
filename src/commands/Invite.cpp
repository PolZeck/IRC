/*
 * This file handles the INVITE command, allowing channel operators to 
 * invite users to join an invite-only (+i) channel.
 */
#include "Server.hpp"

void Server::handleInvite(int fd, std::string args) {
    // Check if the client completed the PASS/NICK/USER registration steps
    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, "451 :You have not registered\r\n");
        return;
    }

    // Validate parameter count: must have at least "nickname channelName"
    size_t space = args.find(' ');
    if (space == std::string::npos) {
        sendResponse(fd, "461 INVITE :Not enough parameters\r\n");
        return;
    }

    std::string targetNick = args.substr(0, space);
    std::string channelName = args.substr(space + 1);

    // Ensure target client exists in our server database
    Client* target = findClientByNick(targetNick);
    if (!target) {
        sendResponse(fd, "401 " + targetNick + " :No such nick/channel\r\n");
        return;
    }

    // Ensure the target channel actually exists
    if (_channels.find(channelName) == _channels.end()) {
        sendResponse(fd, "403 " + channelName + " :No such channel\r\n");
        return;
    }

    Channel* chan = _channels[channelName];

    // Caller must be present in the channel to invite someone
    if (!chan->hasClient(_clients[fd])) {
        sendResponse(fd, "442 " + channelName + " :You're not on that channel\r\n");
        return;
    }

    // Security check: +i channels require operator status to invite
    if (chan->getModeI() && !chan->isOperator(fd)) {
        sendResponse(fd, "482 " + _clients[fd]->getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return;
    }

    // Avoid inviting people who are already inside
    if (chan->hasClient(target)) {
        sendResponse(fd, "443 " + targetNick + " " + channelName + " :is already on channel\r\n");
        return;
    }

    // Add target to the channel's white-list (invite list)
    chan->addInvite(target->getFd());

    // Notify the target and confirm the action to the operator
    std::string inviteMsg = ":" + _clients[fd]->getNickname() + " INVITE " + targetNick + " :" + channelName + "\r\n";
    sendResponse(target->getFd(), inviteMsg);
    sendResponse(fd, "341 " + _clients[fd]->getNickname() + " " + targetNick + " " + channelName + "\r\n");
}