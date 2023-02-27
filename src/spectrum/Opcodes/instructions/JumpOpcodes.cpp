// File: (JumpOpcodes.cpp)
// Created by G.Pimblott on 26/02/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "JumpOpcodes.h"
#include "../../../utils/debug.h"


JumpOpcodes::JumpOpcodes() : OpCodeProvider() {
    createOpCode(JP_XX, "JP_XX", processJP_XX);
}

/**
 * JP XX
 * 10 t-states
 * @param state
 * @return
 */
int JumpOpcodes::processJP_XX(ProcessorState &state) {
    word address = state.getNextWordFromPC();
    debug("JP %#06x\n", address);
    state.setPC(address);
    return 10;
}
