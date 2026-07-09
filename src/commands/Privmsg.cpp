#include "Server.hpp"

void Server::handlePrivmsg(int fd, std::string args) {
    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, "451 :You have not registered\r\n");
        return;
    }

    size_t sep = args.find(':');
    if (args.empty()) {
        sendResponse(fd, "411 :No recipient given (PRIVMSG)\r\n");
        return;
    }

    std::string target = args.substr(0, sep);
    size_t lastSpace = target.find_last_not_of(" ");
    if (lastSpace != std::string::npos)
        target = target.substr(0, lastSpace + 1);
    else
        target = "";

    if (target.empty()) {
        sendResponse(fd, "411 :No recipient given (PRIVMSG)\r\n");
        return;
    }

    if (sep == std::string::npos) {
        sendResponse(fd, "412 :No text to send\r\n");
        return;
    }

    std::string text = args.substr(sep); // Conserve le ':' pour le protocole
    std::string sender = _clients[fd]->getNickname();
    std::string fullMsg = ":" + sender + " PRIVMSG " + target + " " + text + "\r\n";

    if (target[0] == '#') {
        if (_channels.count(target)) {
            if (!_channels[target]->hasClient(_clients[fd])) {
                sendResponse(fd, "404 " + target + " :Cannot send to channel\r\n");
                return;
            }
            broadcastToChannel(_channels[target], fullMsg, fd); // fd en paramètre pour exclure l'émetteur
        } else {
            sendResponse(fd, "403 " + target + " :No such channel\r\n");
        }
    }
    else {
        Client* targetClient = findClientByNick(target);
        if (targetClient) {
            sendResponse(targetClient->getFd(), fullMsg);
        } else {
            sendResponse(fd, "401 " + target + " :No such nick/channel\r\n");
        }
    }
}