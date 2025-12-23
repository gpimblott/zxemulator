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

#include "ArithmeticOpcodes.h"

ArithmeticOpcodes::ArithmeticOpcodes() : OpCodeProvider() {}

int ArithmeticOpcodes::inc8(ProcessorState &state, emulator_types::byte &reg) {
  reg++;

  // Flags: S, Z, H, P/V, N
  // N is reset
  CLEAR_FLAG(N_FLAG, state.registers);

  // Z
  if (reg == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  // S (Sign) - bit 7
  if (reg & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // H (Half Carry) - Carry from bit 3
  if ((reg & 0x0F) == 0x00)
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  // P/V (Overflow) - Detect 0x7F -> 0x80
  if (reg == 0x80)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);

  return 4;
}

int ArithmeticOpcodes::dec8(ProcessorState &state, emulator_types::byte &reg) {
  reg--;

  // Flags: S, Z, H, P/V, N
  // N is set
  SET_FLAG(N_FLAG, state.registers);

  // Z
  if (reg == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  // S
  if (reg & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // H (Half Carry) - Borrow from bit 4
  if ((reg & 0x0F) == 0x0F)
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  // P/V (Overflow) - Detect 0x80 -> 0x7F
  if (reg == 0x7F)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);

  return 4;
}

int ArithmeticOpcodes::processINC_B(ProcessorState &state) {
  return inc8(state, state.registers.B);
}
int ArithmeticOpcodes::processINC_C(ProcessorState &state) {
  return inc8(state, state.registers.C);
}
int ArithmeticOpcodes::processINC_D(ProcessorState &state) {
  return inc8(state, state.registers.D);
}
int ArithmeticOpcodes::processINC_E(ProcessorState &state) {
  return inc8(state, state.registers.E);
}
int ArithmeticOpcodes::processINC_H(ProcessorState &state) {
  return inc8(state, state.registers.H);
}
int ArithmeticOpcodes::processINC_L(ProcessorState &state) {
  return inc8(state, state.registers.L);
}
// int ArithmeticOpcodes::processINC_A(ProcessorState &state) { // Replaced by
// lambda
//   return inc8(state, state.registers.A);
// }

int ArithmeticOpcodes::processDEC_B(ProcessorState &state) {
  return dec8(state, state.registers.B);
}
int ArithmeticOpcodes::processDEC_C(ProcessorState &state) {
  return dec8(state, state.registers.C);
}
int ArithmeticOpcodes::processDEC_D(ProcessorState &state) {
  return dec8(state, state.registers.D);
}
int ArithmeticOpcodes::processDEC_E(ProcessorState &state) {
  return dec8(state, state.registers.E);
}
int ArithmeticOpcodes::processDEC_H(ProcessorState &state) {
  return dec8(state, state.registers.H);
}
int ArithmeticOpcodes::processDEC_L(ProcessorState &state) {
  return dec8(state, state.registers.L);
}
int ArithmeticOpcodes::processDEC_HL_REF(ProcessorState &state) {
  int cycles = dec8(state, state.memory[state.registers.HL]);
  return 11;
}

int ArithmeticOpcodes::processINC_HL_REF(ProcessorState &state) {
  int cycles = inc8(state, state.memory[state.registers.HL]);
  return 11;
}

int ArithmeticOpcodes::inc16(ProcessorState &state, emulator_types::word &reg) {
  reg++;
  return 6;
}

int ArithmeticOpcodes::dec16(ProcessorState &state, emulator_types::word &reg) {
  reg--;
  return 6;
}

int ArithmeticOpcodes::processINC_BC(ProcessorState &state) {
  return inc16(state, state.registers.BC);
}
int ArithmeticOpcodes::processINC_DE(ProcessorState &state) {
  return inc16(state, state.registers.DE);
}
int ArithmeticOpcodes::processINC_HL(ProcessorState &state) {
  return inc16(state, state.registers.HL);
}
int ArithmeticOpcodes::processINC_SP(ProcessorState &state) {
  return inc16(state, state.registers.SP);
}

int ArithmeticOpcodes::processDEC_BC(ProcessorState &state) {
  return dec16(state, state.registers.BC);
}
int ArithmeticOpcodes::processDEC_DE(ProcessorState &state) {
  return dec16(state, state.registers.DE);
}
int ArithmeticOpcodes::processDEC_HL(ProcessorState &state) {
  return dec16(state, state.registers.HL);
}
int ArithmeticOpcodes::processDEC_SP(ProcessorState &state) {
  return dec16(state, state.registers.SP);
}

int ArithmeticOpcodes::add16(ProcessorState &state, emulator_types::word &dest,
                             emulator_types::word src) {
  int result = dest + src;

  // Flags: H, N, C. (S, Z, P/V not affected)
  // N is reset
  CLEAR_FLAG(N_FLAG, state.registers);

  // C: Carry from bit 15
  if (result > 0xFFFF)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);

  // H: Carry from bit 11
  if (((dest & 0x0FFF) + (src & 0x0FFF)) > 0x0FFF)
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  dest = (word)result;
  return 11;
}

int ArithmeticOpcodes::processADD_HL_BC(ProcessorState &state) {
  return add16(state, state.registers.HL, state.registers.BC);
}
int ArithmeticOpcodes::processADD_HL_DE(ProcessorState &state) {
  return add16(state, state.registers.HL, state.registers.DE);
}
int ArithmeticOpcodes::processADD_HL_HL(ProcessorState &state) {
  return add16(state, state.registers.HL, state.registers.HL);
}
int ArithmeticOpcodes::processADD_HL_SP(ProcessorState &state) {
  return add16(state, state.registers.HL, state.registers.SP);
}

int ArithmeticOpcodes::add8(ProcessorState &state, emulator_types::byte val) {
  int res = state.registers.A + val;

  // Flags
  if ((res & 0xFF) == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (res & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
  if (res > 0xFF)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  if (((state.registers.A & 0x0F) + (val & 0x0F)) > 0x0F)
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  // Overflow (P/V)
  // If operands same sign and result different sign -> overflow
  int op1 = (int8_t)state.registers.A;
  int op2 = (int8_t)val;
  int r = (int8_t)(res & 0xFF);
  if (((op1 > 0 && op2 > 0) && r < 0) || ((op1 < 0 && op2 < 0) && r > 0)) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  CLEAR_FLAG(N_FLAG, state.registers);

  state.registers.A = (byte)res;
  return 4;
}

int ArithmeticOpcodes::sub8(ProcessorState &state, emulator_types::byte val) {
  int res = state.registers.A - val;

  // Flags
  if ((res & 0xFF) == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (res & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
  if (state.registers.A < val)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);

  // Half borrow
  if ((state.registers.A & 0x0F) < (val & 0x0F))
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  // Overflow
  int op1 = (int8_t)state.registers.A;
  int op2 = (int8_t)val;
  int r = (int8_t)(res & 0xFF);
  // op1 pos, op2 neg, res neg  OR op1 neg, op2 pos, res pos
  if (((op1 > 0 && op2 < 0) && r < 0) || ((op1 < 0 && op2 > 0) && r > 0)) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG,
               state.registers); // Or maybe just P/V is overflow for SUB? Yes.
  }

  SET_FLAG(N_FLAG, state.registers);

  state.registers.A = (byte)res;
  return 4;
}

int ArithmeticOpcodes::adc8(ProcessorState &state, emulator_types::byte val) {
  int carry = GET_FLAG(C_FLAG, state.registers) ? 1 : 0;
  int res = state.registers.A + val + carry;

  // Flags similar to add8 but with carry
  if ((res & 0xFF) == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else {
    CLEAR_FLAG(Z_FLAG, state.registers);
  }
  if (res & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
  if (res > 0xFF)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  if (((state.registers.A & 0x0F) + (val & 0x0F) + carry) > 0x0F)
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  // Overflow
  int op1 = (int8_t)state.registers.A;
  int op2 = (int8_t)val;
  int r = (int8_t)(res & 0xFF);
  if (((op1 > 0 && op2 > 0) && r < 0) || ((op1 < 0 && op2 < 0) && r > 0)) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  CLEAR_FLAG(N_FLAG, state.registers);
  state.registers.A = (byte)res;
  return 4;
}

int ArithmeticOpcodes::sbc8(ProcessorState &state, emulator_types::byte val) {
  int carry = GET_FLAG(C_FLAG, state.registers) ? 1 : 0;
  int res = state.registers.A - val - carry;

  if ((res & 0xFF) == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (res & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  if ((int)state.registers.A < ((int)val + carry))
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);

  if (((state.registers.A & 0x0F) - (val & 0x0F) - carry) < 0)
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  // Overflow
  int op1 = (int8_t)state.registers.A;
  int op2 =
      (int8_t)val; // We are doing A - val - c. Effectively A + (-val) - c?
  // Overflow logic for SBC is complex. P/V set if result incorrect sign.
  // If operands different signs and result same sign as subtrahend ->
  // overflow? Let's use simplified check or assume standard behavior.

  // For now simple overflow check (same as SUB effectively)
  int r = (int8_t)(res & 0xFF);
  if (((op1 > 0 && op2 < 0) && r < 0) || ((op1 < 0 && op2 > 0) && r > 0)) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  SET_FLAG(N_FLAG, state.registers);
  state.registers.A = (byte)res;
  return 4;
}

int ArithmeticOpcodes::processSCF(ProcessorState &state) {
  SET_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  // P/V, S, Z not affected
  return 4;
}

int ArithmeticOpcodes::processCCF(ProcessorState &state) {
  if (GET_FLAG(C_FLAG, state.registers)) {
    CLEAR_FLAG(C_FLAG, state.registers);
    SET_FLAG(H_FLAG, state.registers);
  } else {
    SET_FLAG(C_FLAG, state.registers);
    CLEAR_FLAG(H_FLAG, state.registers);
  }
  CLEAR_FLAG(N_FLAG, state.registers);
  return 4;
}

int ArithmeticOpcodes::processCPL(ProcessorState &state) {
  state.registers.A = ~state.registers.A;
  SET_FLAG(H_FLAG, state.registers);
  SET_FLAG(N_FLAG, state.registers);
  return 4;
}

int ArithmeticOpcodes::processDAA(ProcessorState &state) {
  byte initA = state.registers.A;
  byte correction = 0;
  bool C = GET_FLAG(C_FLAG, state.registers);
  bool H = GET_FLAG(H_FLAG, state.registers);

  if (H || (initA & 0x0F) > 9) {
    correction += 6;
  }
  if (C || initA > 0x99) {
    correction += 0x60;
    SET_FLAG(C_FLAG, state.registers);
  }

  if (GET_FLAG(N_FLAG, state.registers)) {
    state.registers.A -= correction;
  } else {
    state.registers.A += correction;
  }

  // Flags
  byte res = state.registers.A;
  if (res & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
  if (res == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  // P/V Parity

  // H flag special for DAA..

  return 4;
}
