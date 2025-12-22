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

#include "BitOpcodes.h"

BitOpcodes::BitOpcodes() : OpCodeProvider() {
  createOpCode(0xCB, "CB Prefix", processCB);
}

int BitOpcodes::processCB(ProcessorState &state) {
  // M1 Cycle for second byte
  state.registers.R =
      (state.registers.R & 0x80) | ((state.registers.R + 1) & 0x7F);

  byte opcode = state.getNextByteFromPC();
  state.incPC();

  // Decode operands from opcode
  // x = (opcode >> 6) & 3; // 0=Rotates, 1=BIT, 2=RES, 3=SET
  // y = (opcode >> 3) & 7; // Bit index (for 1,2,3) or Rotate type (for 0)
  // z = opcode & 7;        // Register: 0=B, 1=C, 2=D, 3=E, 4=H, 5=L, 6=(HL),
  // 7=A

  int x = (opcode >> 6) & 3;
  int y = (opcode >> 3) & 7;
  int z = opcode & 7;

  // Identify operand
  byte *regPtr = nullptr;
  byte memVal = 0;
  bool isMem = false;
  word hlAddr = state.registers.HL;

  switch (z) {
  case 0:
    regPtr = &state.registers.B;
    break;
  case 1:
    regPtr = &state.registers.C;
    break;
  case 2:
    regPtr = &state.registers.D;
    break;
  case 3:
    regPtr = &state.registers.E;
    break;
  case 4:
    regPtr = &state.registers.H;
    break;
  case 5:
    regPtr = &state.registers.L;
    break;
  case 6: // (HL)
    isMem = true;
    memVal = state.memory[hlAddr];
    // Create a temporary pointer for logic, but need to write back if modifier
    regPtr = &memVal;
    break;
  case 7:
    regPtr = &state.registers.A;
    break;
  }

  int cycles = 8;
  if (isMem)
    cycles = 12; // (HL) ops usually slower. BIT (HL) is 12, others 15?
  // Rotates: r=8, (HL)=15.
  // BIT: r=8, (HL)=12.
  // SET/RES: r=8, (HL)=15.

  if (x == 0) { // Rotates / Shifts
    if (isMem)
      cycles = 15;

    switch (y) {
    case 0:
      rlc(state, *regPtr);
      break;
    case 1:
      rrc(state, *regPtr);
      break;
    case 2:
      rl(state, *regPtr);
      break;
    case 3:
      rr(state, *regPtr);
      break;
    case 4:
      sla(state, *regPtr);
      break;
    case 5:
      sra(state, *regPtr);
      break;
    case 6:
      sla(state, *regPtr);
      break; // SLL (undocumented) -> usually acts like SLA but shifts in 1
    case 7:
      srl(state, *regPtr);
      break;
    }
  } else if (x == 1) { // BIT
    if (isMem)
      cycles = 12;
    bit(state, y, *regPtr);
    // BIT does NOT write back, so we don't need to update memory for (HL)
    return cycles;
  } else if (x == 2) { // RES
    if (isMem)
      cycles = 15;
    res(y, *regPtr);
  } else if (x == 3) { // SET
    if (isMem)
      cycles = 15;
    set(y, *regPtr);
  }

  // Write back if memory and not BIT
  if (isMem) {
    state.memory[hlAddr] = *regPtr;
  }

  return cycles;
}

void BitOpcodes::rlc(ProcessorState &state, emulator_types::byte &val) {
  int carry = (val & 0x80) ? 1 : 0;
  val = (val << 1) | carry;

  if (carry)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
  // P/V Parity
}

void BitOpcodes::rrc(ProcessorState &state, emulator_types::byte &val) {
  int carry = (val & 0x01) ? 1 : 0;
  val = (val >> 1) | (carry << 7);

  if (carry)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
}

void BitOpcodes::rl(ProcessorState &state, emulator_types::byte &val) {
  int oldCarry = GET_FLAG(C_FLAG, state.registers) ? 1 : 0;
  int newCarry = (val & 0x80) ? 1 : 0;
  val = (val << 1) | oldCarry;

  if (newCarry)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
}

void BitOpcodes::rr(ProcessorState &state, emulator_types::byte &val) {
  int oldCarry = GET_FLAG(C_FLAG, state.registers) ? 1 : 0;
  int newCarry = (val & 0x01) ? 1 : 0;
  val = (val >> 1) | (oldCarry << 7);

  if (newCarry)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
}

void BitOpcodes::sla(ProcessorState &state, emulator_types::byte &val) {
  int carry = (val & 0x80) ? 1 : 0;
  val = val << 1;

  if (carry)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
}

void BitOpcodes::sra(ProcessorState &state, emulator_types::byte &val) {
  int carry = (val & 0x01) ? 1 : 0;
  int msb = val & 0x80;
  val = (val >> 1) | msb; // Keep MSB (Arithmetic shift)

  if (carry)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
}

void BitOpcodes::srl(ProcessorState &state, emulator_types::byte &val) {
  int carry = (val & 0x01) ? 1 : 0;
  val = val >> 1; // 0 into MSB

  if (carry)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
}

void BitOpcodes::bit(ProcessorState &state, int bit, emulator_types::byte val) {
  bool z = !((val >> bit) & 1);
  if (z)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  SET_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  // S, P/V flags are officially 'unknown', but often P/V=Z?
}

void BitOpcodes::set(int bit, emulator_types::byte &val) { val |= (1 << bit); }

void BitOpcodes::res(int bit, emulator_types::byte &val) { val &= ~(1 << bit); }
