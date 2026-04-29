#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <iostream>
#include <map>

class Client {
private:
    int         _fd;
    std::string _nickname;
    std::string _buffer; // Memory for partial data

public:
    Client(int fd) : _fd(fd), _nickname("") {}
    ~Client() {}

    int         getFd() const { return _fd; }
    void        appendBuffer(std::string str) { _buffer += str; }
    std::string getBuffer() const { return _buffer; }
    void        clearBuffer() { _buffer.clear(); }
    
    // Logic to check if a command is complete (ends with \n or \r\n)
    bool        hasCommand() const {
        return (_buffer.find('\n') != std::string::npos);
    }
};

#endif