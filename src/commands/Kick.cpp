#include "Server.hpp"

void Server::handleKick(int fd, std::string args) {
    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, "451 :You have not registered\r\n");
        return;
    }

    size_t space1 = args.find(' ');
    if (space1 == std::string::npos) {
        sendResponse(fd, "461 KICK :Not enough parameters\r\n");
        return;
    }

    std::string channelName = args.substr(0, space1);
    std::string rest = args.substr(space1 + 1);
    
    size_t space2 = rest.find(' ');
    std::string targetNick = (space2 == std::string::npos) ? rest : rest.substr(0, space2);

    // I check if the channel exists
    if (_channels.find(channelName) == _channels.end()) {
        sendResponse(fd, "403 " + channelName + " :No such channel\r\n");
        return;
    }

    // I use my new utility function to find the victim
    Client* target = findClientByNick(targetNick);
    if (!target || !_channels[channelName]->hasClient(target)) {
        sendResponse(fd, "441 " + targetNick + " " + channelName + " :They aren't on that channel\r\n");
        return;
    }

    // I notify everyone and remove the user
    std::string kickMsg = ":" + _clients[fd]->getNickname() + " KICK " + channelName + " " + targetNick + " :Kicked by operator\r\n";
    _channels[channelName]->broadcast(kickMsg);
    _channels[channelName]->removeClient(target);
}