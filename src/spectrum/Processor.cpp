// File: (Processor.cpp)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "Processor.h"

/**
 * Set the initial state of the processor ready to start executing
 */
void Processor::init(const char *romFile) {
    // Setup the default state of the registers
    state.registers.pc = 0x0;     // Set the initial execution position to 0

    // LoadFactory the ROM into memory
    Rom theROM = Rom(romFile);
    state.memory.loadIntoMemory(theROM);

    state.memory.dump(0, 16);

    // Ready to GO :)
}

void Processor::run() {
    int count = 0;
    running = true;
    while (running) {
        // need some clock cycle stuff here
        count++;
        if (count >= 2) running = false;

        OpCode *opCode = getNextInstruction();
        if (opCode != nullptr) {
            printf("Found : %s\n", opCode->getName().c_str());
            opCode->execute();
        }
    }
}

void Processor::shutdown() {

}

/**
 * Read the next instruction and process it
 * @return
 */
OpCode *Processor::getNextInstruction() {
    byte opcode = state.memory[state.registers.pc++];
    return catalogue.lookup(opcode);
}
