/*
 * This file implements the PRIVMSG command for private messaging or channel broadcasts.
 */
#include "Server.hpp"

void Server::handlePrivmsg(int fd, std::string args) {
    std::string nick = _clients[fd]->getNickname().empty() ? "*" : _clients[fd]->getNickname();

    if (!_clients[fd]->isRegistered()) {
        sendResponse(fd, ":localhost 451 " + nick + " :You have not registered\r\n");
        return;
    }

    // Parse the target and the message text (separated by ':')
    size_t sep = args.find(':');
    if (args.empty()) {
        sendResponse(fd, ":localhost 411 " + nick + " :No recipient given (PRIVMSG)\r\n");
        return;
    }

    std::string target = args.substr(0, sep);
    size_t lastSpace = target.find_last_not_of(" ");
    if (lastSpace != std::string::npos)
        target = target.substr(0, lastSpace + 1);
    else
        target = "";

    if (target.empty() || sep == std::string::npos) {
        if (target.empty()) {
            sendResponse(fd, ":localhost 411 " + nick + " :No recipient given\r\n");
        } else {
            sendResponse(fd, ":localhost 412 " + nick + " :No text to send\r\n");
        }
        return;
    }

    std::string text = args.substr(sep); // Keep the colon for RFC compliance
    std::string userMask = nick + "!" + _clients[fd]->getUsername() + "@localhost";
    std::string fullMsg = ":" + userMask + " PRIVMSG " + target + " " + text + "\r\n";

    // Route to channel if target starts with '#', otherwise perform private chat
    if (target[0] == '#') {
        if (_channels.count(target)) {
            if (!_channels[target]->hasClient(_clients[fd])) {
                sendResponse(fd, ":localhost 404 " + nick + " " + target + " :Cannot send to channel\r\n");
                return;
            }
            broadcastToChannel(_channels[target], fullMsg, fd); // Exclude sender from broadcast
        } else {
            sendResponse(fd, ":localhost 403 " + nick + " " + target + " :No such channel\r\n");
        }
    } else {
        Client* targetClient = findClientByNick(target);
        if (targetClient) {
            sendResponse(targetClient->getFd(), fullMsg);
        } else {
            sendResponse(fd, ":localhost 401 " + nick + " " + target + " :No such nick/channel\r\n");
        }
    }
}