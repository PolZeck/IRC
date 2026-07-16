/*
 * This file manages the KICK command for removing users from a channel.
 */
#include "Server.hpp"

void Server::handleKick(int fd, std::string args) {
    std::string nick = _clients[fd]->getNickname().empty() ? "*" : _clients[fd]->getNickname();

    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, ":localhost 451 " + nick + " :You have not registered\r\n");
        return;
    }

    size_t space1 = args.find(' ');
    if (space1 == std::string::npos) {
        sendResponse(fd, ":localhost 461 " + nick + " KICK :Not enough parameters\r\n");
        return;
    }

    std::string channelName = args.substr(0, space1);
    std::string rest = args.substr(space1 + 1);
    
    size_t space2 = rest.find(' ');
    std::string targetNick = (space2 == std::string::npos) ? rest : rest.substr(0, space2);

    if (_channels.find(channelName) == _channels.end()) {
        sendResponse(fd, ":localhost 403 " + nick + " " + channelName + " :No such channel\r\n");
        return;
    }

    Channel* chan = _channels[channelName];

    if (!chan->isOperator(fd)) {
        sendResponse(fd, ":localhost 482 " + nick + " " + channelName + " :You're not channel operator\r\n");
        return;
    }

    Client* target = findClientByNick(targetNick);
    if (!target || !chan->hasClient(target)) {
        sendResponse(fd, ":localhost 441 " + nick + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n");
        return;
    }

    // Handle optional reason for the kick, defaulting to "Kicked by operator" if none is provided
    std::string reason = "Kicked by operator";
    if (space2 != std::string::npos) {
        reason = rest.substr(space2 + 1);
        if (!reason.empty() && reason[0] == ':') {
            reason = reason.substr(1);
        }
    }

    std::string userMask = nick + "!" + _clients[fd]->getUsername() + "@localhost";
    std::string kickMsg = ":" + userMask + " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";
    broadcastToChannel(chan, kickMsg, -1);

    chan->removeClient(target);
}