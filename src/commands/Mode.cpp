#include "Server.hpp"
#include <sstream>
#include <cstdlib>

void Server::handleMode(int fd, std::string args) {
    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, "451 :You have not registered\r\n");
        return;
    }

    std::stringstream ss(args);
    std::string target, modeString, param;
    ss >> target >> modeString;

    // If it's a nickname mode request, we safely ignore it (out of scope for basic mandatory 42)
    if (!target.empty() && target[0] != '#') return;

    if (_channels.find(target) == _channels.end()) {
        sendResponse(fd, "403 " + target + " :No such channel\r\n");
        return;
    }

    Channel* chan = _channels[target];

    // If no modes are specified, clients want to know current active flags
    if (modeString.empty()) {
        std::string activeModes = "+";
        if (chan->getModeI()) activeModes += "i";
        if (chan->getModeT()) activeModes += "t";
        if (!chan->getKey().empty()) activeModes += "k";
        if (chan->getMaxClients() > 0) activeModes += "l";
        sendResponse(fd, "324 " + _clients[fd]->getNickname() + " " + target + " " + activeModes + "\r\n");
        return;
    }

    // Check if the user is allowed to alter settings
    if (!chan->isOperator(fd)) {
        sendResponse(fd, "482 " + _clients[fd]->getNickname() + " " + target + " :You're not channel operator\r\n");
        return;
    }

    bool sign = true; // true = '+', false = '-'
    for (size_t i = 0; i < modeString.length(); i++) {
        char c = modeString[i];
        if (c == '+') { sign = true; continue; }
        if (c == '-') { sign = false; continue; }

        if (c == 'i') chan->setModeI(sign);
        else if (c == 't') chan->setModeT(sign);
        else if (c == 'k') {
            if (sign) {
                ss >> param;
                if (!param.empty()) chan->setKey(param);
            } else {
                chan->setKey("");
            }
        }
        else if (c == 'l') {
            if (sign) {
                ss >> param;
                if (!param.empty()) chan->setMaxClients(std::atoi(param.c_str()));
            } else {
                chan->setMaxClients(0);
            }
        }
        else if (c == 'o') {
            ss >> param;
            Client* tClient = findClientByNick(param);
            if (tClient && chan->hasClient(tClient)) {
                if (sign) chan->addOperator(tClient->getFd());
                else chan->removeOperator(tClient->getFd());
            } else {
                sendResponse(fd, "441 " + param + " " + target + " :They aren't on that channel\r\n");
            }
        }
    }
    // Broadcast the mode update to everyone in the channel
    std::string updateMsg = ":" + _clients[fd]->getNickname() + " MODE " + target + " " + modeString + (param.empty() ? "" : " " + param) + "\r\n";
    chan->broadcast(updateMsg);
}