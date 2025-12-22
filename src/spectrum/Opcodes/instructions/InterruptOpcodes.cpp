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

#include "InterruptOpcodes.h"

InterruptOpcodes::InterruptOpcodes() : OpCodeProvider() {
  createOpCode(DI, "DI", processDI);
  createOpCode(EI, "EI", processEI);
  createOpCode(0x76, "HALT", [](ProcessorState &state) {
    state.setHalted(true);
    // HALT executes NOPs while waiting.
    // PC is NOT incremented past next instruction until interrupt?
    // Actually Z80 HALT: PC stays at the instruction FOLLOWING the HALT?
    // No, HALT executes repeatedly (4 T-states) until interrupt.
    // PC points to instruction AFTER HALT.
    // But the CPU doesn't fetch it.
    // When interrupt comes, it pushes PC (next instruction) and jumps to ISR.
    // So here we should NOT increment PC?
    // Our `Processor` loop increments PC *after* getting instruction but
    // *before* execute? "this->state.registers.PC++;" in Processor.cpp. So PC
    // already points to next instruction. We just set halted=true to stop
    // fetching.
    return 4;
  });
};

/**
 * DI - Disable Interrupts
 * 4 t states
 * @param state
 * @return
 */
int InterruptOpcodes::processDI(ProcessorState &state) {
  state.setInterrupts(false);
  // debug("%s", "Interrupts disabled\n");
  return 4;
};

/**
 * EI - Enable Interrupts
 * 4 t states
 * @param state
 * @return
 */
int InterruptOpcodes::processEI(ProcessorState &state) {
  state.setInterrupts(true);
  // debug("%s", "EI Executed: Interrupts enabled\n");
  return 4;
}
