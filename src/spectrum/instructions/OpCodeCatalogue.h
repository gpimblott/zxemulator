// File: (OpCodeCatalogue.h)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_OPCODECATALOGUE_H
#define ZXEMULATOR_OPCODECATALOGUE_H

#include <map>
#include "../../utils/BaseTypes.h"
#include "OpCode.h"

typedef std::map<emulator_types::byte,OpCode*> catalogue_t;

class OpCodeCatalogue {
private:
    catalogue_t catalogue;

public:
    OpCodeCatalogue(ProcessorState & state);
    OpCode *lookup(emulator_types::byte opcode);

    void add(OpCode *opcode);

    virtual ~OpCodeCatalogue();

};


#endif //ZXEMULATOR_OPCODECATALOGUE_H
