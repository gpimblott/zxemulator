//
// Created by gordo on 12/01/2023.
//

#include <iostream>
#include "Logger.h"

using namespace std;

namespace utils {

    void Logger::write(const char *message) {
        cout << message << std::endl;
    }

} // utils