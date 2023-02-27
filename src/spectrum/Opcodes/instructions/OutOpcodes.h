// File: (OutOpcodes.h)
// Created by G.Pimblott on 26/02/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_OUTOPCODES_H
#define ZXEMULATOR_OUTOPCODES_H


#include "../OpCodeProvider.h"
#include "../../../utils/BaseTypes.h"

class OutOpcodes : public OpCodeProvider {
private:
    emulator_types::byte OUT = 0xd3;

    static int processOUT(ProcessorState &state);

public:
    OutOpcodes();
};


#endif //ZXEMULATOR_OUTOPCODES_H
