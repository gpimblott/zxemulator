// File: (LogicOpcodes.cpp)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include <cstdio>
#include "LogicOpcodes.h"
#include "../../../utils/debug.h"

LogicOpcodes::LogicOpcodes() : OpCodeProvider() {
    createOpCode(XOR_A, "XOR_A", processXOR_A);
};

/**
 *
 * XOR A - 0xAF
 * 4 t-states
 *
 * C and N flags cleared. P/V is parity, and rest are modified by definition.
 *
 * @param state
 * @return
 */
int LogicOpcodes::processXOR_A(ProcessorState &state) {
    CLEAR_FLAG(C_FLAG, state.registers);
    CLEAR_FLAG(N_FLAG, state.registers);
    state.registers.A ^= state.registers.A;

    debug("XOR A result=%#02x\n", state.registers.A);
    return 4;
}


