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

#include "LogicOpcodes.h"

LogicOpcodes::LogicOpcodes()
    : OpCodeProvider() {

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

  // debug("XOR A result=%#02x\n", state.registers.A);
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
