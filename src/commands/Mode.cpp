/*
 * This file implements the MODE command to manage channel settings and flags.
 */
#include "Server.hpp"
#include <sstream>
#include <cstdlib>

void Server::handleMode(int fd, std::string args) {
    std::string nick = _clients[fd]->getNickname().empty() ? "*" : _clients[fd]->getNickname();

    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, ":localhost 451 " + nick + " :You have not registered\r\n");
        return;
    }

    std::stringstream ss(args);
    std::string target, modeString;
    ss >> target >> modeString;

    if (!target.empty() && target[0] != '#') return;

    if (_channels.find(target) == _channels.end()) {
        sendResponse(fd, ":localhost 403 " + nick + " " + target + " :No such channel\r\n");
        return;
    }

    Channel* chan = _channels[target];

    if (modeString.empty()) {
        std::string activeModes = "+";
        if (chan->getModeI()) activeModes += "i";
        if (chan->getModeT()) activeModes += "t";
        if (!chan->getKey().empty()) activeModes += "k";
        if (chan->getMaxClients() > 0) activeModes += "l";
        sendResponse(fd, ":localhost 324 " + nick + " " + target + " " + activeModes + "\r\n");
        return;
    }

    if (!chan->isOperator(fd)) {
        sendResponse(fd, ":localhost 482 " + nick + " " + target + " :You're not channel operator\r\n");
        return;
    }

    std::string appliedModes = "";
    std::string appliedParams = "";
    bool sign = true;

    for (size_t i = 0; i < modeString.length(); i++) {
        char c = modeString[i];
        if (c == '+') { sign = true; continue; }
        if (c == '-') { sign = false; continue; }

        std::string param = "";
        if (c == 'i') { chan->setModeI(sign); appliedModes += (sign ? "+i" : "-i"); }
        else if (c == 't') { chan->setModeT(sign); appliedModes += (sign ? "+t" : "-t"); }
        else if (c == 'k') {
            if (sign) { ss >> param; if (!param.empty()) chan->setKey(param); }
            else { chan->setKey(""); }
            appliedModes += (sign ? "+k" : "-k");
        }
        else if (c == 'l') {
            if (sign) { ss >> param; if (!param.empty()) chan->setMaxClients(std::atoi(param.c_str())); }
            else { chan->setMaxClients(0); }
            appliedModes += (sign ? "+l" : "-l");
        }
        else if (c == 'o') {
            ss >> param;
            if (!param.empty()) {
                Client* tClient = findClientByNick(param);
                if (tClient && chan->hasClient(tClient)) {
                    if (sign) chan->addOperator(tClient->getFd()); else chan->removeOperator(tClient->getFd());
                    appliedModes += (sign ? "+o" : "-o");
                    appliedParams += " " + param;
                }
            }
        }
    }

    if (!appliedModes.empty()) {
        std::string userMask = nick + "!" + _clients[fd]->getUsername() + "@localhost";
        std::string updateMsg = ":" + userMask + " MODE " + target + " " + appliedModes + appliedParams + "\r\n";
        broadcastToChannel(chan, updateMsg, -1);
    }
}