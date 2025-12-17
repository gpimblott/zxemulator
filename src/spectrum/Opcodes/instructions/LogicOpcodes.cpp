// File: (LogicOpcodes.cpp)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "LogicOpcodes.h"
#include "../../../utils/debug.h"
#include <cstdio>

LogicOpcodes::LogicOpcodes() : OpCodeProvider() {
  createOpCode(XOR_A, "XOR_A", processXOR_A);
  createOpCode(CP_N, "CP_N", processCP_N);
  createOpCode(NOP, "NOP", processNOP);

  createOpCode(CP_B, "CP_B", processCP_B);
  createOpCode(CP_C, "CP_C", processCP_C);
  createOpCode(CP_D, "CP_D", processCP_D);
  createOpCode(CP_E, "CP_E", processCP_E);
  createOpCode(CP_H, "CP_H", processCP_H);
  createOpCode(CP_L, "CP_L", processCP_L);
  createOpCode(CP_HL, "CP_HL", processCP_HL);
  createOpCode(CP_A, "CP_A", processCP_A);

  createOpCode(AND_B, "AND_B", processAND_B);
  createOpCode(AND_C, "AND_C", processAND_C);
  createOpCode(AND_D, "AND_D", processAND_D);
  createOpCode(AND_E, "AND_E", processAND_E);
  createOpCode(AND_H, "AND_H", processAND_H);
  createOpCode(AND_L, "AND_L", processAND_L);
  createOpCode(AND_HL, "AND_HL", processAND_HL);
  createOpCode(AND_A, "AND_A", processAND_A);

  createOpCode(XOR_B, "XOR_B", processXOR_B);
  createOpCode(XOR_C, "XOR_C", processXOR_C);
  createOpCode(XOR_D, "XOR_D", processXOR_D);
  createOpCode(XOR_E, "XOR_E", processXOR_E);
  createOpCode(XOR_H, "XOR_H", processXOR_H);
  createOpCode(XOR_L, "XOR_L", processXOR_L);
  createOpCode(XOR_HL, "XOR_HL", processXOR_HL);

  createOpCode(OR_B, "OR_B", processOR_B);
  createOpCode(OR_C, "OR_C", processOR_C);
  createOpCode(OR_D, "OR_D", processOR_D);
  createOpCode(OR_E, "OR_E", processOR_E);
  createOpCode(OR_H, "OR_H", processOR_H);
  createOpCode(OR_L, "OR_L", processOR_L);
  createOpCode(OR_HL, "OR_HL", processOR_HL);
  createOpCode(OR_A, "OR_A", processOR_A);
};

/**
 *
 * XOR A - 0xAF
 * 4 t-states
 *
 * C and N flags cleared. P/V is parity, and rest are modified by definition.
 *
 * @param state
 * @return
 */
int LogicOpcodes::processXOR_A(ProcessorState &state) {
  CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  state.registers.A ^= state.registers.A;

  debug("XOR A result=%#02x\n", state.registers.A);
  return 4;
}

/**
 * NOP - No Operation
 * 4 t-states
 */
int LogicOpcodes::processNOP(ProcessorState &state) {
  // debug("NOP\n");
  return 4;
}

/**
 * Compare (SUB but discard result)
 * CP n (0xFE)
 * 7 t-states
 */
int LogicOpcodes::processCP_N(ProcessorState &state) {
  byte value = state.getNextByteFromPC();
  state.incPC();
  return cp8(state, value);
}

int LogicOpcodes::cp8(ProcessorState &state, emulator_types::byte value) {
  // Perform sub (A - n)
  int result = state.registers.A - value;

  // Set Flags
  // S, Z, H, P/V, N, C

  // Z: Zero Flag
  if ((result & 0xFF) == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  // C: Carry Flag (if A < n then carry)
  if (state.registers.A < value)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);

  // N: Add/Subtract Flag (Set for SUB/CP)
  SET_FLAG(N_FLAG, state.registers);

  // TODO: S, H, P/V flags

  // debug("CP %02X (A=%02X)\n", value, state.registers.A);
  return 4;
}

int LogicOpcodes::processCP_B(ProcessorState &state) {
  return cp8(state, state.registers.B);
}
int LogicOpcodes::processCP_C(ProcessorState &state) {
  return cp8(state, state.registers.C);
}
int LogicOpcodes::processCP_D(ProcessorState &state) {
  return cp8(state, state.registers.D);
}
int LogicOpcodes::processCP_E(ProcessorState &state) {
  return cp8(state, state.registers.E);
}
int LogicOpcodes::processCP_H(ProcessorState &state) {
  return cp8(state, state.registers.H);
}
int LogicOpcodes::processCP_L(ProcessorState &state) {
  return cp8(state, state.registers.L);
}
int LogicOpcodes::processCP_A(ProcessorState &state) {
  return cp8(state, state.registers.A);
}

int LogicOpcodes::processCP_HL(ProcessorState &state) {
  byte val = state.memory[state.registers.HL];
  int cycles = cp8(state, val);
  return 7; // CP (HL) takes 7 t-states, cp8 returns 4 default
}

int LogicOpcodes::and8(ProcessorState &state, emulator_types::byte value) {
  state.registers.A &= value;

  // Flags: S, Z, H, P/V, N, C
  // C is reset, N is reset, H is set
  CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  SET_FLAG(H_FLAG, state.registers);

  if (state.registers.A == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  if (state.registers.A & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // TODO: P/V parity

  return 4;
}

int LogicOpcodes::xor8(ProcessorState &state, emulator_types::byte value) {
  state.registers.A ^= value;

  // Flags: S, Z, H, P/V, N, C
  // C is reset, N is reset, H is reset
  CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);

  if (state.registers.A == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  if (state.registers.A & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // TODO: P/V parity

  return 4;
}

int LogicOpcodes::or8(ProcessorState &state, emulator_types::byte value) {
  state.registers.A |= value;

  // Flags: S, Z, H, P/V, N, C
  // C is reset, N is reset, H is reset
  CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);

  if (state.registers.A == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  if (state.registers.A & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // TODO: P/V parity

  return 4;
}

int LogicOpcodes::processAND_B(ProcessorState &state) {
  return and8(state, state.registers.B);
}
int LogicOpcodes::processAND_C(ProcessorState &state) {
  return and8(state, state.registers.C);
}
int LogicOpcodes::processAND_D(ProcessorState &state) {
  return and8(state, state.registers.D);
}
int LogicOpcodes::processAND_E(ProcessorState &state) {
  return and8(state, state.registers.E);
}
int LogicOpcodes::processAND_H(ProcessorState &state) {
  return and8(state, state.registers.H);
}
int LogicOpcodes::processAND_L(ProcessorState &state) {
  return and8(state, state.registers.L);
}
int LogicOpcodes::processAND_A(ProcessorState &state) {
  return and8(state, state.registers.A);
}
int LogicOpcodes::processAND_HL(ProcessorState &state) {
  byte val = state.memory[state.registers.HL];
  int cycles = and8(state, val);
  return 7;
}

int LogicOpcodes::processXOR_B(ProcessorState &state) {
  return xor8(state, state.registers.B);
}
int LogicOpcodes::processXOR_C(ProcessorState &state) {
  return xor8(state, state.registers.C);
}
int LogicOpcodes::processXOR_D(ProcessorState &state) {
  return xor8(state, state.registers.D);
}
int LogicOpcodes::processXOR_E(ProcessorState &state) {
  return xor8(state, state.registers.E);
}
int LogicOpcodes::processXOR_H(ProcessorState &state) {
  return xor8(state, state.registers.H);
}
int LogicOpcodes::processXOR_L(ProcessorState &state) {
  return xor8(state, state.registers.L);
}
int LogicOpcodes::processXOR_HL(ProcessorState &state) {
  byte val = state.memory[state.registers.HL];
  int cycles = xor8(state, val);
  return 7;
}

int LogicOpcodes::processOR_B(ProcessorState &state) {
  return or8(state, state.registers.B);
}
int LogicOpcodes::processOR_C(ProcessorState &state) {
  return or8(state, state.registers.C);
}
int LogicOpcodes::processOR_D(ProcessorState &state) {
  return or8(state, state.registers.D);
}
int LogicOpcodes::processOR_E(ProcessorState &state) {
  return or8(state, state.registers.E);
}
int LogicOpcodes::processOR_H(ProcessorState &state) {
  return or8(state, state.registers.H);
}
int LogicOpcodes::processOR_L(ProcessorState &state) {
  return or8(state, state.registers.L);
}
int LogicOpcodes::processOR_A(ProcessorState &state) {
  return or8(state, state.registers.A);
}
int LogicOpcodes::processOR_HL(ProcessorState &state) {
  byte val = state.memory[state.registers.HL];
  int cycles = or8(state, val);
  return 7;
}
