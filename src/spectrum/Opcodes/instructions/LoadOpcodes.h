// File: (LoadOpcodes.h)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_LOADOPCODES_H
#define ZXEMULATOR_LOADOPCODES_H

#include "../../../utils/BaseTypes.h"
#include "../OpCode.h"
#include "../OpCodeProvider.h"

class LoadOpcodes : public OpCodeProvider {
private:
    static constexpr emulator_types::byte LD_DE_XX = 0x11;
    static constexpr emulator_types::byte LD_B_A = 0x47;
    static constexpr emulator_types::byte LD_A_X = 0x3e;

    static int processLD_DE_XX(ProcessorState &state);

    static int processLD_B_A(ProcessorState &state);

    static int processLD_A_X(ProcessorState &state);

public:
    LoadOpcodes();
};


#endif //ZXEMULATOR_LOADOPCODES_H
