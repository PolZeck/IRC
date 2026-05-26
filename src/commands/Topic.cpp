#include "Server.hpp"

void Server::handleTopic(int fd, std::string args) {
    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, "451 :You have not registered\r\n");
        return;
    }

    size_t space = args.find(' ');
    std::string channelName = (space == std::string::npos) ? args : args.substr(0, space);

    if (_channels.find(channelName) == _channels.end()) {
        sendResponse(fd, "403 " + channelName + " :No such channel\r\n");
        return;
    }

    Channel* chan = _channels[channelName];

    // Scenario 1: Just reading the topic
    if (space == std::string::npos) {
        if (chan->getTopic().empty())
            sendResponse(fd, "331 " + _clients[fd]->getNickname() + " " + channelName + " :No topic is set\r\n");
        else
            sendResponse(fd, "332 " + _clients[fd]->getNickname() + " " + channelName + " :" + chan->getTopic() + "\r\n");
        return;
    }

    // Scenario 2: Changing the topic
    std::string newTopic = args.substr(space + 1);
    if (newTopic[0] == ':') newTopic = newTopic.substr(1);

    // I enforce the +t policy here
    if (chan->getModeT() && !chan->isOperator(fd)) {
        sendResponse(fd, "482 " + _clients[fd]->getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return;
    }

    chan->setTopic(newTopic);
    std::string msg = ":" + _clients[fd]->getNickname() + " TOPIC " + channelName + " :" + newTopic + "\r\n";
    chan->broadcast(msg);
}