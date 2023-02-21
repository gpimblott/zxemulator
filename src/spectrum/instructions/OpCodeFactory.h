// File: (OpCodeFactory.h)
// Created by G.Pimblott on 19/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_OPCODEFACTORY_H
#define ZXEMULATOR_OPCODEFACTORY_H


#include "OpCodeCatalogue.h"

class OpCodeFactory  {
private:
    ProcessorState &state;

protected:
    OpCode *createOpCode( emulator_types::byte, std::string name, executeFunc_t func);

public:
    OpCodeFactory(ProcessorState &state) : state(state) {
    }

    virtual void addToCatalogue(OpCodeCatalogue &catalogue) = 0;
};


#endif //ZXEMULATOR_OPCODEFACTORY_H
