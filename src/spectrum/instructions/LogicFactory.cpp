// File: (LogicFactory.cpp)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include <cstdio>
#include "LogicFactory.h"

namespace OpCodes {

    LogicFactory::LogicFactory(ProcessorState &state) : OpCodeFactory(state) {
    };

    /**
     * Add the specific opcode processors
     * @param catalogue
     */
    void LogicFactory::addToCatalogue(OpCodeCatalogue &catalogue) {
        catalogue.add(createOpCode(XOR_A, "XOR_A", processXOR_A));
    }

    /**
     *
     * @param state
     * @return
     */
    int LogicFactory::processXOR_A(ProcessorState &state) {
        printf("Processing XORA");
        return 0;
    }


}

