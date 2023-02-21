// File: (Processor.h)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_PROCESSOR_H
#define ZXEMULATOR_PROCESSOR_H

#include "instructions/OpCodeCatalogue.h"
#include "ProcessorTypes.h"

class Processor {

private:
    // State variables
    ProcessorState state;
    OpCodeCatalogue catalogue = OpCodeCatalogue(state);

    bool running = false;

    // Internal methods
    OpCode *getNextInstruction();

public:
    void init(const char *romFile);

    void run();

    void shutdown();
};


#endif //ZXEMULATOR_PROCESSOR_H
