// File: (ProcessorState.h)
// Created by G.Pimblott on 26/02/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_PROCESSORSTATE_H
#define ZXEMULATOR_PROCESSORSTATE_H


#include "ProcessorTypes.h"
#include "Memory.h"

class ProcessorState {
private:
    bool interruptsEnabled = true;
public:
    Z80Registers registers;
    Memory memory;

    // Supporting routines
    void setInterrupts(bool value);
    word getNextWordFromPC();
    byte getNextByteFromPC();

    // Program counter util functions
    long incPC();
    long incPC(int value);
    long setPC(long address);

};


#endif //ZXEMULATOR_PROCESSORSTATE_H
