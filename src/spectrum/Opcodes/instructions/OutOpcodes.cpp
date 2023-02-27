// File: (OutOpcodes.cpp)
// Created by G.Pimblott on 26/02/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "OutOpcodes.h"
#include "../../../utils/debug.h"


OutOpcodes::OutOpcodes() : OpCodeProvider() {
    createOpCode(OUT, "OUT", processOUT);
}


int OutOpcodes::processOUT(ProcessorState &state) {
    debug("OUT ");
    return 0;
}
