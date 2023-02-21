//
// Created by gordo on 13/01/2023.
//

#ifndef ZXEMULATOR_REGISTERUTILS_H
#define ZXEMULATOR_REGISTERUTILS_H

#include "../spectrum/ProcessorTypes.h"

#define setAF( reg , value)  reg.A =

/**
 * Utility class to work with the registers
 */
class RegisterUtils {

public:
    static void test(Z80Registers &registers);

    static void printAF(const Z80Registers &registers);
};

#endif //ZXEMULATOR_REGISTERUTILS_H
