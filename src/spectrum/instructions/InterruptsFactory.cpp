// File: (InterruptsFactory.cpp)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include <cstdio>
#include "InterruptsFactory.h"

namespace OpCodes {
    InterruptsFactory::InterruptsFactory(ProcessorState &state) : OpCodeFactory(state) {
    };

    /**
     * Add the specific opcode processors
     * @param catalogue
     */
    void InterruptsFactory::addToCatalogue(OpCodeCatalogue &catalogue) {
        catalogue.add(createOpCode(DI, "DI", processDI ));
    }


    /**
     *
     * @param state
     * @return
     */
    int InterruptsFactory::processDI(ProcessorState &state) {
        printf("Processing DI\n");
        return 0;
    }

}
