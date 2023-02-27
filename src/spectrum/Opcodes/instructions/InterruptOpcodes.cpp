// File: (InterruptOpcodes.cpp)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include <cstdio>
#include "../../../utils/debug.h"
#include "InterruptOpcodes.h"


InterruptOpcodes::InterruptOpcodes() : OpCodeProvider() {
    createOpCode(DI, "DI", processDI);
};

/**
 * DI - Disable Interrupts
 * 4 t states
 * @param state
 * @return
 */
int InterruptOpcodes::processDI(ProcessorState &state) {
    state.setInterrupts(false);
    debug("%s", "Interrupts disabled\n");
    return 4;
};

