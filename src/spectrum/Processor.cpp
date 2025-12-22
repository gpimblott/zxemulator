/*
 * MIT License
 *
 * Copyright (c) 2026 G.Pimblott
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "Processor.h"
#include "../utils/debug.h"

Processor::Processor() : state() {
  // Set up the default state of the registers
  reset();
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

  // Fire an interrupt
  if (!paused && state.areInterruptsEnabled()) {
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

  while (tStates < frameCycles && running) {
    if (paused) {
      if (stepRequest) {
        stepRequest = false;
      } else {
        break;
      }
    }

    if (state.isHalted()) {
      // CPU executes NOPs (4 T-states) while halted
      tStates += 4;
      continue; // Skip fetch/execute
    }

    OpCode *opCode = getNextInstruction();
    if (opCode != nullptr) {
      // increment past the opcode
      this->state.registers.PC++;

      tStates += opCode->execute(this->state);
    } else {
      byte unknownOpcode = state.memory[state.registers.PC];
      char errorMsg[100];
      snprintf(errorMsg, sizeof(errorMsg), "Unknown opcode %02X at address %d",
               unknownOpcode, this->state.registers.PC);
      lastError = std::string(errorMsg);

      debug("Unknown opcode %02X at address %d\n", unknownOpcode,
            this->state.registers.PC);
      running = false;
    }
  }
}

VideoBuffer *Processor::getVideoBuffer() {
  return state.memory.getVideoBuffer();
}

void Processor::shutdown() {}

void Processor::reset() {
  state.registers.PC = 0x0;
  state.registers.AF = 0xFFFF;
  state.registers.SP = 0xFFFF;
  state.registers.BC = 0;
  state.registers.DE = 0;
  state.registers.HL = 0;
  state.registers.IX = 0;
  state.registers.IY = 0;
  state.setHalted(false);
  state.setInterrupts(false);
  lastError = "";
}

/**
 * Read the next instruction and process it
 * @return
 */
OpCode *Processor::getNextInstruction() {

  byte opcode = state.memory[state.registers.PC];
  return catalogue.lookupOpcode(opcode);
}
