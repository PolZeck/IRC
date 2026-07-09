#include "Channel.hpp"

Channel::Channel(std::string name) : _name(name), _topic(""), _key(""), _maxClients(0), _modeI(false), _modeT(false) {}

Channel::~Channel() {}

void Channel::addClient(Client* client) {
    _clients.push_back(client);
    if (_clients.size() == 1) {
        addOperator(client->getFd());
    }
}

void Channel::removeClient(Client* client) {
    for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if ((*it)->getFd() == client->getFd()) {
            _clients.erase(it);
            break;
        }
    }
    removeOperator(client->getFd());
}

bool Channel::hasClient(Client* client) {
    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i]->getFd() == client->getFd()) return true;
    }
    return false;
}

void Channel::addOperator(int fd) {
    if (!isOperator(fd)) _operators.push_back(fd);
}

void Channel::removeOperator(int fd) {
    std::vector<int>::iterator it = std::find(_operators.begin(), _operators.end(), fd);
    if (it != _operators.end()) _operators.erase(it);
}

bool Channel::isOperator(int fd) {
    return std::find(_operators.begin(), _operators.end(), fd) != _operators.end();
}

void Channel::addInvite(int fd) {
    if (!isInvited(fd)) _invitedFds.push_back(fd);
}

bool Channel::isInvited(int fd) {
    return std::find(_invitedFds.begin(), _invitedFds.end(), fd) != _invitedFds.end();
}

