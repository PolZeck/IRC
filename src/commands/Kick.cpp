/*
 * This file manages the KICK command for removing users from a channel.
 */
#include "Server.hpp"

void Server::handleKick(int fd, std::string args) {
    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, "451 :You have not registered\r\n");
        return;
    }

    // Split logic: channel name + target nickname
    size_t space1 = args.find(' ');
    if (space1 == std::string::npos) {
        sendResponse(fd, "461 KICK :Not enough parameters\r\n");
        return;
    }

    std::string channelName = args.substr(0, space1);
    std::string rest = args.substr(space1 + 1);
    
    size_t space2 = rest.find(' ');
    std::string targetNick = (space2 == std::string::npos) ? rest : rest.substr(0, space2);

    if (_channels.find(channelName) == _channels.end()) {
        sendResponse(fd, "403 " + channelName + " :No such channel\r\n");
        return;
    }

    Channel* chan = _channels[channelName];

    // Security: Only operators are allowed to KICK members
    if (!chan->isOperator(fd)) {
        sendResponse(fd, "482 " + _clients[fd]->getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return;
    }

    // Ensure target exists and is currently in the channel
    Client* target = findClientByNick(targetNick);
    if (!target || !chan->hasClient(target)) {
        sendResponse(fd, "441 " + targetNick + " " + channelName + " :They aren't on that channel\r\n");
        return;
    }

    // Announce the kick to everyone in the channel
    std::string kickMsg = ":" + _clients[fd]->getNickname() + " KICK " + channelName + " " + targetNick + "\r\n";
    broadcastToChannel(chan, kickMsg, -1);

    // Perform the removal
    chan->removeClient(target);
}