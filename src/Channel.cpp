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