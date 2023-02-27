// File: (LogicOpcodes.h)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_LOGICOPCODES_H
#define ZXEMULATOR_LOGICOPCODES_H

#include "../../../utils/BaseTypes.h"
#include "../OpCode.h"
#include "../OpCodeProvider.h"

class LogicOpcodes : public OpCodeProvider {
private:
    emulator_types::byte XOR_A = 0xaf;

    static int processXOR_A(ProcessorState &state);

public:
    LogicOpcodes();

};


#endif //ZXEMULATOR_LOGICOPCODES_H
