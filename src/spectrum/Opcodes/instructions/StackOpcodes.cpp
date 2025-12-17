// File: (StackOpcodes.cpp)
#include "StackOpcodes.h"
#include "../../../utils/debug.h"

StackOpcodes::StackOpcodes() : OpCodeProvider() {
  createOpCode(PUSH_BC, "PUSH BC", processPUSH_BC);
  createOpCode(PUSH_DE, "PUSH DE", processPUSH_DE);
  createOpCode(PUSH_HL, "PUSH HL", processPUSH_HL);
  createOpCode(PUSH_AF, "PUSH AF", processPUSH_AF);

  createOpCode(POP_BC, "POP BC", processPOP_BC);
  createOpCode(POP_DE, "POP DE", processPOP_DE);
  createOpCode(POP_HL, "POP HL", processPOP_HL);
  createOpCode(POP_AF, "POP AF", processPOP_AF);
}

void StackOpcodes::push(ProcessorState &state, emulator_types::word val) {
  state.registers.SP--;
  state.memory[state.registers.SP] = (emulator_types::byte)((val >> 8) & 0xFF);
  state.registers.SP--;
  state.memory[state.registers.SP] = (emulator_types::byte)(val & 0xFF);
}

emulator_types::word StackOpcodes::pop(ProcessorState &state) {
  emulator_types::byte low = state.memory[state.registers.SP];
  state.registers.SP++;
  emulator_types::byte high = state.memory[state.registers.SP];
  state.registers.SP++;
  return (high << 8) | low;
}

int StackOpcodes::processPUSH_BC(ProcessorState &state) {
  push(state, state.registers.BC);
  return 11;
}
int StackOpcodes::processPUSH_DE(ProcessorState &state) {
  push(state, state.registers.DE);
  return 11;
}
int StackOpcodes::processPUSH_HL(ProcessorState &state) {
  push(state, state.registers.HL);
  return 11;
}
int StackOpcodes::processPUSH_AF(ProcessorState &state) {
  push(state, state.registers.AF);
  return 11;
}

int StackOpcodes::processPOP_BC(ProcessorState &state) {
  state.registers.BC = pop(state);
  return 10;
}
int StackOpcodes::processPOP_DE(ProcessorState &state) {
  state.registers.DE = pop(state);
  return 10;
}
int StackOpcodes::processPOP_HL(ProcessorState &state) {
  state.registers.HL = pop(state);
  return 10;
}
int StackOpcodes::processPOP_AF(ProcessorState &state) {
  state.registers.AF = pop(state);
  return 10;
}
