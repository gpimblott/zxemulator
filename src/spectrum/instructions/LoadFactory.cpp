// File: (LoadFactory.cpp)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include <cstdio>
#include "LoadFactory.h"

namespace OpCodes {

    LoadFactory::LoadFactory(ProcessorState &state) : OpCodeFactory(state) {
    }

    /**
 * Add the specific opcode processors
 * @param catalogue
 */
    void LoadFactory::addToCatalogue(OpCodeCatalogue &catalogue) {
        catalogue.add(createOpCode(LD_DE_XX, "LD_DE_XX", processLD_DE_XX));
    }

    int LoadFactory::processLD_DE_XX(ProcessorState &state) {
        printf("Processing LD_DE_XX");
        return 0;
    }

} // OpCodes