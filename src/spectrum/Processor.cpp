// File: (Processor.cpp)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "Processor.h"
#include "../utils/debug.h"

Processor::Processor() : state() {
    // Set up the default state of the registers
    state.registers.PC = 0x0;     // Set the initial execution position to 0
    state.registers.AF = 0xFFFF;
    state.registers.SP = 0xFFFF;
}

/**
 * initialise the processor with a ROM file
 */
void Processor::init(const char *romFile) {
    // Load the ROM into memory
    Rom theROM = Rom(romFile);
    state.memory.loadIntoMemory(theROM);

    state.memory.dump(0, 32);

    // Ready to GO :)
}

void Processor::run() {
    int count = 0;
    running = true;
    while (running) {
        // need some clock cycle stuff here
        count++;

        OpCode *opCode = getNextInstruction();
        if (opCode != nullptr) {
            // increment past the opcode
            this->state.registers.PC++;

            // execute the opcode
            opCode->execute(this->state);
        } else {
            debug( "Unknown opcode at address %d", this->state.registers.PC);
            running = false;
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
#ifdef DEBUG
    state.memory.dump(state.registers.PC, 8);
#endif

    byte opcode = state.memory[state.registers.PC];
    return catalogue.lookupOpcode(opcode);
}
