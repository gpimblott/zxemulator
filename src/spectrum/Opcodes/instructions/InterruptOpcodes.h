// File: (InterruptOpcodes.h)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_INTERRUPTOPCODES_H
#define ZXEMULATOR_INTERRUPTOPCODES_H

#include "../../../utils/BaseTypes.h"
#include "../OpCode.h"
#include "../OpCodeProvider.h"

class InterruptOpcodes : public OpCodeProvider {
private:
    static constexpr emulator_types::byte DI = 0xf3;

    static int processDI(ProcessorState &state);

public:
    InterruptOpcodes();
};


#endif //ZXEMULATOR_INTERRUPTOPCODES_H
