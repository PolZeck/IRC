#include "Server.hpp"
#include <sstream>

void Server::handleJoin(int fd, std::string args) {
    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, "451 :You have not registered\r\n");
        return;
    }

    std::stringstream ss(args);
    std::string channelName, providedKey;
    ss >> channelName >> providedKey;

    if (channelName.empty() || channelName[0] != '#') {
        sendResponse(fd, "461 JOIN :Not enough parameters or invalid channel name\r\n");
        return;
    }

    bool isCreator = false;
    // Création dynamique si le canal n'existe pas encore
    if (_channels.find(channelName) == _channels.end()) {
        _channels[channelName] = new Channel(channelName);
        isCreator = true; // On flag pour lui attribuer les privilèges OP plus bas
    }

    Channel* chan = _channels[channelName];

    // 1. Vérification du mode Invite-only (+i)
    if (chan->getModeI() && !chan->isInvited(fd)) {
        sendResponse(fd, "473 " + _clients[fd]->getNickname() + " " + channelName + " :Cannot join channel (+i)\r\n");
        return;
    }

    // 2. Vérification du mode Clé/Password (+k)
    if (!chan->getKey().empty() && chan->getKey() != providedKey) {
        sendResponse(fd, "475 " + _clients[fd]->getNickname() + " " + channelName + " :Cannot join channel (+k)\r\n");
        return;
    }

    // 3. Vérification de la limite d'utilisateurs (+l)
    if (chan->getMaxClients() > 0 && chan->getClientCount() >= chan->getMaxClients()) {
        sendResponse(fd, "471 " + _clients[fd]->getNickname() + " " + channelName + " :Cannot join channel (+l)\r\n");
        return;
    }

    // Ajout du client au canal
    chan->addClient(_clients[fd]);

    // Si c'est lui qui a créé le canal, il devient opérateur automatiquement
    if (isCreator) {
        chan->addOperator(fd);
    }

    std::string joinMsg = ":" + _clients[fd]->getNickname() + " JOIN " + channelName + "\r\n";
    chan->broadcast(joinMsg);
}