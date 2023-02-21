//
// Created by gordo on 13/01/2023.
//

#include <stdio.h>
#include "RegisterUtils.h"

using namespace emulator_types;
/**
 * Test that the little endian works
 * @param registers
 */
void RegisterUtils::test(Z80Registers &registers) {
    registers.A = (byte)0xE5;
    registers.F = (byte)0x5A;

    printf("Testing A & F\n");
    printf("Ans A=E5 F=5A - AF=5AE5\n");
    printAF(registers);

    *(registers.AF) = 0x5AE5; // E5 in A; 5A in F

    printf("Testing AF access storing 5AE5\n");
    printf("Answer A=E5 F=5A\n");
    printAF(registers);

}

void RegisterUtils::printAF(const Z80Registers &registers) {
    byte A = registers.A;
    byte F = registers.F;
    word AF = *registers.AF;

    printf( "A=%hhx,F=%hhx - AF=%x\n\n", A , F , AF );
}

