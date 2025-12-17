// File: (Processor.cpp)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "Processor.h"
#include "../utils/debug.h"

Processor::Processor() : state() {
  // Set up the default state of the registers
  state.registers.PC = 0x0; // Set the initial execution position to 0
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
  running = true;
}

void Processor::run() {
  running = true;
  while (running) {
    executeFrame();
  }
}

void Processor::executeFrame() {
  // 3.5MHz * 0.02s (50Hz) ~= 69888 T-states per frame
  int tStates = 0;
  const int frameCycles = 69888;

  while (tStates < frameCycles && running) {
    if (state.isHalted()) {
      // CPU executes NOPs (4 T-states) while halted
      tStates += 4;
      continue; // Skip fetch/execute
    }

    OpCode *opCode = getNextInstruction();
    if (opCode != nullptr) {
      // increment past the opcode
      this->state.registers.PC++;

      // execute the opcode
      tStates += opCode->execute(this->state);
    } else {
      byte unknownOpcode = state.memory[state.registers.PC];
      debug("Unknown opcode %02X at address %d\n", unknownOpcode,
            this->state.registers.PC);
      running = false;
    }
  }

  // Fire an interrupt
  if (state.areInterruptsEnabled()) {
    // If we were halted, we are no longer halted
    if (state.isHalted()) {
      state.setHalted(false);
      // PC already points to next instruction (instruction following HALT)
    }

    // Push PC
    state.registers.SP -= 2;
    word pc = state.registers.PC;
    state.memory[state.registers.SP] = (byte)(pc & 0xFF);
    state.memory[state.registers.SP + 1] = (byte)((pc >> 8) & 0xFF);

    // Jump to 0x0038 (IM1 default)
    state.registers.PC = 0x0038;

    // Disable interrupts (standard Z80 behavior on accept)
    state.setInterrupts(false);
  }
}

VideoBuffer *Processor::getVideoBuffer() {
  return state.memory.getVideoBuffer();
}

void Processor::shutdown() {}

/**
 * Read the next instruction and process it
 * @return
 */
OpCode *Processor::getNextInstruction() {

  byte opcode = state.memory[state.registers.PC];
  return catalogue.lookupOpcode(opcode);
}
