/*
 * This file implements the TOPIC command to set or view channel topics.
 */
#include "Server.hpp"

void Server::handleTopic(int fd, std::string args) {
    std::string nick = _clients[fd]->getNickname().empty() ? "*" : _clients[fd]->getNickname();

    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, ":localhost 451 " + nick + " :You have not registered\r\n");
        return;
    }

    size_t space = args.find(' ');
    std::string channelName = (space == std::string::npos) ? args : args.substr(0, space);

    if (_channels.find(channelName) == _channels.end()) {
        sendResponse(fd, ":localhost 403 " + nick + " " + channelName + " :No such channel\r\n");
        return;
    }

    Channel* chan = _channels[channelName];

    // No topic provided: Return current channel topic
    if (space == std::string::npos) {
        if (chan->getTopic().empty())
            sendResponse(fd, ":localhost 331 " + nick + " " + channelName + " :No topic is set\r\n");
        else
            sendResponse(fd, ":localhost 332 " + nick + " " + channelName + " :" + chan->getTopic() + "\r\n");
        return;
    }

    // Setting new topic: Check permissions if +t mode is active
    std::string newTopic = args.substr(space + 1);
    if (!newTopic.empty() && newTopic[0] == ':') newTopic = newTopic.substr(1);

    if (chan->getModeT() && !chan->isOperator(fd)) {
        sendResponse(fd, ":localhost 482 " + nick + " " + channelName + " :You're not channel operator\r\n");
        return;
    }

    chan->setTopic(newTopic);

    // Broadcast the new topic to everyone in the channel with the user's hostmask
    std::string userMask = nick + "!" + _clients[fd]->getUsername() + "@localhost";
    std::string topicBroadcast = ":" + userMask + " TOPIC " + channelName + " :" + newTopic + "\r\n";
    broadcastToChannel(chan, topicBroadcast, -1);
}