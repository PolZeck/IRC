#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <signal.h>

// Global variable to allow the signal handler to stop the server loop
// This is one of the few cases where a global is acceptable in C++
bool g_stop = false;

void handleSignal(int sig) {
    (void)sig;
    std::cout << "\nStopping the server..." << std::endl;
    g_stop = true;
}

int main(int argc, char **argv) {
    // 1. Check arguments: ./ircserv <port> <password>
    if (argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }

    // 2. Parse port and password
    int port = std::atoi(argv[1]);
    std::string password = argv[2];

    if (port < 1024 || port > 65535) {
        std::cerr << "Error: Invalid port. Please use a port between 1024 and 65535." << std::endl;
        return 1;
    }

    if (password.empty()) {
        std::cerr << "Error: Password cannot be empty." << std::endl;
        return 1;
    }

    // 3. Setup signal handling (to avoid leaks on Ctrl+C)
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);

    // 4. Launch Server
    try {
        Server ircServer(port, password);
        
        std::cout << "--- IRC Server Started ---" << std::endl;
        std::cout << "Port: " << port << std::endl;
        std::cout << "Password: " << password << std::endl;

        ircServer.init(); // Create socket, bind, listen
        ircServer.run();  // Main loop with poll()
        
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
