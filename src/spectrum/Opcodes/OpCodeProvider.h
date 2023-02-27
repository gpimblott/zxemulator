// File: (OpCodeProvider.h)
// Created by G.Pimblott on 19/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_OPCODEPROVIDER_H
#define ZXEMULATOR_OPCODEPROVIDER_H

#include <map>
#include "OpCode.h"

class OpCodeProvider  {
private:
    std::map<emulator_types::byte, OpCode *> opcodeLookup;
protected:
    OpCode *createOpCode( emulator_types::byte, std::string name, executeFunc_t func);

public:
    OpCodeProvider() {
    }

    OpCode *lookupOpcode(byte code);
};


#endif //ZXEMULATOR_OPCODEPROVIDER_H
