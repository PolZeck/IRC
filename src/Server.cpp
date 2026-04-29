#include "Server.hpp"

Server::Server(int port, std::string password) : _port(port), _password(password), _serverFd(-1) {
    // Initialisation basique
}

Server::~Server() {
    // Penser à fermer les sockets ici plus tard
}

void Server::init() {
    std::cout << "Server initialization logic goes here..." << std::endl;
    // Ici il faudra faire : socket(), bind(), listen()
}

void Server::run() {
    std::cout << "Server is now running and waiting for connections..." << std::endl;
    // Ici il faudra faire la boucle poll()
}