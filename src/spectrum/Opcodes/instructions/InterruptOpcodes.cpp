// File: (InterruptOpcodes.cpp)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "InterruptOpcodes.h"
#include "../../../utils/debug.h"
#include <cstdio>

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
  debug("%s", "Interrupts disabled\n");
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
  debug("%s", "Interrupts enabled\n");
  return 4;
}
