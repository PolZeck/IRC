#include "Server.hpp"

void Server::handleJoin(int fd, std::string args) {
    // I only allow registered clients to join channels
    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, "451 :You have not registered\r\n");
        return;
    }
    // IRC channel names must start with #
    if (args.empty() || args[0] != '#') {
        sendResponse(fd, "461 JOIN :Not enough parameters or invalid channel name\r\n");
        return;
    }

    std::string channelName = args;
    // If the channel doesn't exist in my map, I create it on the fly
    if (_channels.find(channelName) == _channels.end()) {
        _channels[channelName] = new Channel(channelName);
        std::cout << "Server: Created new channel " << channelName << std::endl;
    }

    // I add the client to the channel's member list
    _channels[channelName]->addClient(_clients[fd]);

    // I notify everyone in the channel about the new member
    std::string joinMsg = ":" + _clients[fd]->getNickname() + " JOIN " + channelName + "\r\n";
    _channels[channelName]->broadcast(joinMsg);
}