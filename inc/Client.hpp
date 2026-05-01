#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <iostream>
#include <map>

class Client {
    private:
        int         _fd;
        std::string _buffer; // Accumulates data until a full command (\n) is received 
        
        bool _passwordOk;
        bool _isRegistered;
        std::string _nickname;

    public:
        // Client(int fd) : _fd(fd) {}

        Client(int fd) : _fd(fd), _passwordOk(false), _isRegistered(false) {}
    
        void setPasswordOk(bool state) { _passwordOk = state; }
        bool isPasswordOk() const { return _passwordOk; }
        
        void setNickname(std::string nick) { _nickname = nick; }
        std::string getNickname() const { return _nickname; }
        int getFd() const { return _fd; }
        
        std::string getBuffer() const { return _buffer; }
        void        clearBuffer() { _buffer.clear(); }

        bool isRegistered() const { return _isRegistered; }
        void setRegistered(bool state) { _isRegistered = state; }
        
        // Concatenate new raw data to the existing buffer [cite: 142]
        void appendBuffer(std::string str) { _buffer += str; }
        
        // Search for the end-of-command delimiter (\n) 
        bool hasCommand() const {
            return (_buffer.find('\n') != std::string::npos);
        }
        
        // Extraction logic: returns the first full command found in the buffer [cite: 142]
        std::string extractNextCommand() {
            size_t pos = _buffer.find('\n');
            std::string cmd = _buffer.substr(0, pos); // Get everything before \n
            _buffer.erase(0, pos + 1);                // Remove the extracted part from buffer
            return cmd;
        }
};

#endif