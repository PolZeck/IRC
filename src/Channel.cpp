#include "Channel.hpp"
#include <sys/socket.h>
#include <unistd.h>     

Channel::Channel(std::string name) : _name(name) {}

Channel::~Channel() {}

std::string Channel::getName() const { return _name; }

void Channel::addClient(Client* client) {
    _clients.push_back(client);
}

void Channel::broadcast(std::string message, int excludeFd) {
    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i]->getFd() != excludeFd) {
            send(_clients[i]->getFd(), message.c_str(), message.length(), 0);
        }
    }
}

const std::vector<Client*>& Channel::getClients() const { return _clients; }

void Channel::removeClient(Client* client) {
    for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if ((*it)->getFd() == client->getFd()) {
            _clients.erase(it);
            break;
        }
    }
}

bool Channel::hasClient(Client* client) {
    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i]->getFd() == client->getFd())
            return true;
    }
    return false;
}

bool Channel::isEmpty() const {
    return _clients.empty();
}