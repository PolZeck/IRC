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

    // Si c'est une requête sur un utilisateur (et pas un canal), on ignore gentiment (hors scope)
    if (!target.empty() && target[0] != '#') return;

    if (_channels.find(target) == _channels.end()) {
        sendResponse(fd, "403 " + target + " :No such channel\r\n");
        return;
    }

    Channel* chan = _channels[target];

    // Si aucun mode n'est spécifié, le client demande la liste des flags actifs
    if (modeString.empty()) {
        std::string activeModes = "+";
        if (chan->getModeI()) activeModes += "i";
        if (chan->getModeT()) activeModes += "t";
        if (!chan->getKey().empty()) activeModes += "k";
        if (chan->getMaxClients() > 0) activeModes += "l";
        sendResponse(fd, "324 " + _clients[fd]->getNickname() + " " + target + " " + activeModes + "\r\n");
        return;
    }

    // Sécurité stricte du sujet : Seuls les opérateurs peuvent modifier les paramètres
    if (!chan->isOperator(fd)) {
        sendResponse(fd, "482 " + _clients[fd]->getNickname() + " " + target + " :You're not channel operator\r\n");
        return;
    }

    std::string appliedModes = "";
    std::string appliedParams = "";
    bool sign = true; // true = '+', false = '-'

    for (size_t i = 0; i < modeString.length(); i++) {
        char c = modeString[i];
        if (c == '+') { 
            sign = true; 
            if (appliedModes.empty() || appliedModes[appliedModes.size() - 1] != '+') appliedModes += "+";
            continue; 
        }
        if (c == '-') { 
            sign = false; 
            if (appliedModes.empty() || appliedModes[appliedModes.size() - 1] != '-') appliedModes += "-";
            continue; 
        }

        std::string param = "";

        if (c == 'i') {
            chan->setModeI(sign);
            appliedModes += "i";
        }
        else if (c == 't') {
            chan->setModeT(sign);
            appliedModes += "t";
        }
        else if (c == 'k') {
            if (sign) {
                ss >> param;
                if (!param.empty()) {
                    chan->setKey(param);
                    appliedModes += "k";
                    appliedParams += " " + param;
                }
            } else {
                chan->setKey("");
                appliedModes += "k";
            }
        }
        else if (c == 'l') {
            if (sign) {
                ss >> param;
                if (!param.empty()) {
                    chan->setMaxClients(std::atoi(param.c_str()));
                    appliedModes += "l";
                    appliedParams += " " + param;
                }
            } else {
                chan->setMaxClients(0);
                appliedModes += "l";
            }
        }
        else if (c == 'o') {
            ss >> param;
            if (!param.empty()) {
                Client* tClient = findClientByNick(param);
                if (tClient && chan->hasClient(tClient)) {
                    if (sign) chan->addOperator(tClient->getFd());
                    else chan->removeOperator(tClient->getFd());
                    appliedModes += "o";
                    appliedParams += " " + param;
                } else {
                    sendResponse(fd, "441 " + param + " " + target + " :They aren't on that channel\r\n");
                }
            }
        }
    }

    // On ne broadcast que si des modes valides ont effectivement été traités
    if (appliedModes != "+" && appliedModes != "-") {
        std::string updateMsg = ":" + _clients[fd]->getNickname() + " MODE " + target + " " + appliedModes + appliedParams + "\r\n";
        chan->broadcast(updateMsg);
    }
}