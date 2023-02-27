//
// Created by gordo on 13/01/2023.
//

#include <stdio.h>
#include "RegisterUtils.h"

using namespace emulator_types;


void RegisterUtils::printAF(const Z80Registers &registers) {
    byte A = registers.A;
    byte F = registers.F;
    word AF = registers.AF;

    printf( "A=%hhx,F=%hhx - AF=%x\n\n", A , F , AF );
}

