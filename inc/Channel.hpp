#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <vector>
#include <string>
#include <algorithm>
#include <sys/socket.h>
#include "Client.hpp"

class Channel {
    private:
        std::string             _name;
        std::string             _topic;
        std::string             _key;
        size_t                  _maxClients;
        
        // Flags de modes
        bool                    _modeI; // Invite only
        bool                    _modeT; // Restricted topic
        
        std::vector<Client*>    _clients;
        std::vector<int>        _operators; // Stocke les FDs des opérateurs (@)
        std::vector<int>        _invitedFds; // Stocke les FDs invités

    public:
        Channel(std::string name);
        ~Channel();

        std::string getName() const { return _name; }
        std::string getTopic() const { return _topic; }
        void setTopic(std::string topic) { _topic = topic; }
        
        // Getters / Setters pour les modes
        bool getModeI() const { return _modeI; }
        void setModeI(bool val) { _modeI = val; }
        bool getModeT() const { return _modeT; }
        void setModeT(bool val) { _modeT = val; }
        std::string getKey() const { return _key; }
        void setKey(std::string key) { _key = key; }
        size_t getMaxClients() const { return _maxClients; }
        void setMaxClients(size_t limit) { _maxClients = limit; }

        // Gestion des membres
        void addClient(Client* client);
        void removeClient(Client* client);
        bool hasClient(Client* client);
        bool isEmpty() const { return _clients.empty(); }
        size_t getClientCount() const { return _clients.size(); }

        // Gestion des privilèges
        void addOperator(int fd);
        void removeOperator(int fd);
        bool isOperator(int fd);

        // Gestion des invitations
        void addInvite(int fd);
        bool isInvited(int fd);

        const std::vector<Client*>& getClients() const { return _clients; }
};

#endif