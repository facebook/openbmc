#pragma once
#include <string>
#include <exception>

class SELException : public std::exception {
public:
    SELException(const std::string message): msg(message){};
    virtual ~SELException(){};
    virtual const char* what() const throw() {return msg.c_str();};
private:
    std::string msg;
};

class SELParserError : public SELException {
public:
    SELParserError(const std::string message): SELException(message){}
    virtual ~SELParserError(){};
};
