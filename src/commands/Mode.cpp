/*
 * This file implements the MODE command to manage channel settings and flags.
 */
#include "Server.hpp"
#include <sstream>
#include <cstdlib>

void Server::handleMode(int fd, std::string args) {
    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, "451 :You have not registered\r\n");
        return;
    }

    std::stringstream ss(args);
    std::string target, modeString;
    ss >> target >> modeString;

    if (!target.empty() && target[0] != '#') return;

    if (_channels.find(target) == _channels.end()) {
        sendResponse(fd, "403 " + target + " :No such channel\r\n");
        return;
    }

    Channel* chan = _channels[target];

    // If no mode string provided, return the 324 RPL (Channel Mode List)
    if (modeString.empty()) {
        std::string activeModes = "+";
        if (chan->getModeI()) activeModes += "i";
        if (chan->getModeT()) activeModes += "t";
        if (!chan->getKey().empty()) activeModes += "k";
        if (chan->getMaxClients() > 0) activeModes += "l";
        sendResponse(fd, "324 " + _clients[fd]->getNickname() + " " + target + " " + activeModes + "\r\n");
        return;
    }

    // Only operators can change channel modes
    if (!chan->isOperator(fd)) {
        sendResponse(fd, "482 " + _clients[fd]->getNickname() + " " + target + " :You're not channel operator\r\n");
        return;
    }

    std::string appliedModes = "";
    std::string appliedParams = "";
    bool sign = true; // State toggle: '+' (true) or '-' (false)

    for (size_t i = 0; i < modeString.length(); i++) {
        char c = modeString[i];
        if (c == '+') { sign = true; continue; } // Switch mode sign
        if (c == '-') { sign = false; continue; }

        std::string param = "";
        // Process standard modes (i, t, k, l, o)
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
    // Broadcast the successful mode change to the channel members
    if (!appliedModes.empty()) {
        std::string updateMsg = ":" + _clients[fd]->getNickname() + " MODE " + target + " " + appliedModes + appliedParams + "\r\n";
        broadcastToChannel(chan, updateMsg, -1);
    }
}