// File: (LoadOpcodes.cpp)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include <cstdio>
#include "LoadOpcodes.h"
#include "../../../utils/debug.h"


/**
 * Add the opcodes to the catalogue
 * @param state
 * @param catalogue
 */
LoadOpcodes::LoadOpcodes() : OpCodeProvider() {
    createOpCode(LD_DE_XX, "LD_DE_XX", processLD_DE_XX);
    createOpCode(LD_B_A, "LD_B_A", processLD_B_A);
    createOpCode(LD_A_X, "LD_A_X", processLD_A_X);
}

/**
 * Load a value into DE (word)
 * 10 t-states
 * @param state
 * @return
 */
int LoadOpcodes::processLD_DE_XX(ProcessorState &state) {
    // Get the next word and put it in DE
    word value = state.getNextWordFromPC();
    debug("LD DE,%#06x\n", value);
    state.registers.DE = value;
    state.incPC(2);
    return 10;
}

/**
 * Load the value of A into B
 * 4 t-states
 * @param state
 * @return
 */
int LoadOpcodes::processLD_B_A(ProcessorState &state) {
    state.registers.B = state.registers.A;
    debug("LD B, A (%#04x)\n", state.registers.B);
    return 4;
}

/**
 * Load the value of A into B
 * 4 t-states
 * @param state
 * @return
 */
int LoadOpcodes::processLD_A_X(ProcessorState &state) {
    byte value = state.getNextByteFromPC();
    state.registers.A = value;
    state.incPC();
    debug("LD A, %#04x\n", value);
    return 4;
}
