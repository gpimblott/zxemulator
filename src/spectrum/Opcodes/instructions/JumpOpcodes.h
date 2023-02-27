// File: (JumpOpcodes.h)
// Created by G.Pimblott on 26/02/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_JUMPOPCODES_H
#define ZXEMULATOR_JUMPOPCODES_H

#include "../OpCodeProvider.h"
#include "../../../utils/BaseTypes.h"

class JumpOpcodes : public OpCodeProvider {
private:
    static constexpr emulator_types::byte JP_XX = 0xc3;

    static int processJP_XX(ProcessorState &state);

public:
    JumpOpcodes();
};


#endif //ZXEMULATOR_JUMPOPCODES_H
