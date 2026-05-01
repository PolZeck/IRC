#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include "Client.hpp"

class Channel {
private:
    std::string             _name;
    std::vector<Client*>    _clients;

public:
    Channel(std::string name);
    ~Channel();

    std::string getName() const;
    void        addClient(Client* client);
    void        removeClient(int fd);
    void        broadcast(std::string message, int excludeFd = -1);
    
    const std::vector<Client*>& getClients() const;
};

#endif