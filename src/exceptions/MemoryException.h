// File: (MemoryException.h)
// Created by G.Pimblott on 17/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_MEMORYEXCEPTION_H
#define ZXEMULATOR_MEMORYEXCEPTION_H

#include <exception>
#include <string>

class MemoryException : public std::exception {
private:
    std::string message;
public:
    MemoryException(std::string msg) {
        this->message = "Memory exception: " + msg;
    };

    MemoryException(long address) {
        this->message = std::string("Memory exception address=") + std::to_string(address);
    }

    const char *what() {
        return this->message.c_str();
    }
};


#endif //ZXEMULATOR_MEMORYEXCEPTION_H
