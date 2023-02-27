// File: (OpCodeCatalogue.h)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_OPCODECATALOGUE_H
#define ZXEMULATOR_OPCODECATALOGUE_H

#include <list>
#include "OpCode.h"
#include "../../utils/BaseTypes.h"
#include "../ProcessorState.h"
#include "OpCodeProvider.h"
#include "instructions/LoadOpcodes.h"

typedef std::list<OpCodeProvider *> providerList_t;

class OpCodeCatalogue {
private:
    providerList_t providersList;

public:
    OpCodeCatalogue();

    void add(OpCodeProvider *provider);

    OpCode *lookupOpcode(byte opcode);

    virtual ~OpCodeCatalogue();
};


#endif //ZXEMULATOR_OPCODECATALOGUE_H
