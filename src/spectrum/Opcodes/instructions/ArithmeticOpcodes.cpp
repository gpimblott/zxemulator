// File: (ArithmeticOpcodes.cpp)
// Created by Antigravity

#include "ArithmeticOpcodes.h"
#include "../../../utils/debug.h"

ArithmeticOpcodes::ArithmeticOpcodes() : OpCodeProvider() {
  createOpCode(ADD_HL_BC, "ADD_HL_BC", processADD_HL_BC);
  createOpCode(ADD_HL_DE, "ADD_HL_DE", processADD_HL_DE);
  createOpCode(ADD_HL_HL, "ADD_HL_HL", processADD_HL_HL);
  createOpCode(ADD_HL_SP, "ADD_HL_SP", processADD_HL_SP);

  // ADD A, r -> 0x80 - 0x87
  createOpCode(0x80, "ADD A, B",
               [](ProcessorState &s) { return add8(s, s.registers.B); });
  createOpCode(0x81, "ADD A, C",
               [](ProcessorState &s) { return add8(s, s.registers.C); });
  createOpCode(0x82, "ADD A, D",
               [](ProcessorState &s) { return add8(s, s.registers.D); });
  createOpCode(0x83, "ADD A, E",
               [](ProcessorState &s) { return add8(s, s.registers.E); });
  createOpCode(0x84, "ADD A, H",
               [](ProcessorState &s) { return add8(s, s.registers.H); });
  createOpCode(0x85, "ADD A, L",
               [](ProcessorState &s) { return add8(s, s.registers.L); });
  createOpCode(0x86, "ADD A, (HL)", [](ProcessorState &s) {
    return add8(s, s.memory[s.registers.HL]);
  });
  createOpCode(0x87, "ADD A, A",
               [](ProcessorState &s) { return add8(s, s.registers.A); });

  // ADC A, r -> 0x88 - 0x8F
  createOpCode(0x88, "ADC A, B",
               [](ProcessorState &s) { return adc8(s, s.registers.B); });
  createOpCode(0x89, "ADC A, C",
               [](ProcessorState &s) { return adc8(s, s.registers.C); });
  createOpCode(0x8A, "ADC A, D",
               [](ProcessorState &s) { return adc8(s, s.registers.D); });
  createOpCode(0x8B, "ADC A, E",
               [](ProcessorState &s) { return adc8(s, s.registers.E); });
  createOpCode(0x8C, "ADC A, H",
               [](ProcessorState &s) { return adc8(s, s.registers.H); });
  createOpCode(0x8D, "ADC A, L",
               [](ProcessorState &s) { return adc8(s, s.registers.L); });
  createOpCode(0x8E, "ADC A, (HL)", [](ProcessorState &s) {
    return adc8(s, s.memory[s.registers.HL]);
  });
  createOpCode(0x8F, "ADC A, A",
               [](ProcessorState &s) { return adc8(s, s.registers.A); });

  // SUB r -> 0x90 - 0x97
  createOpCode(0x90, "SUB B",
               [](ProcessorState &s) { return sub8(s, s.registers.B); });
  createOpCode(0x91, "SUB C",
               [](ProcessorState &s) { return sub8(s, s.registers.C); });
  createOpCode(0x92, "SUB D",
               [](ProcessorState &s) { return sub8(s, s.registers.D); });
  createOpCode(0x93, "SUB E",
               [](ProcessorState &s) { return sub8(s, s.registers.E); });
  createOpCode(0x94, "SUB H",
               [](ProcessorState &s) { return sub8(s, s.registers.H); });
  createOpCode(0x95, "SUB L",
               [](ProcessorState &s) { return sub8(s, s.registers.L); });
  createOpCode(0x96, "SUB (HL)", [](ProcessorState &s) {
    return sub8(s, s.memory[s.registers.HL]);
  });
  createOpCode(0x97, "SUB A",
               [](ProcessorState &s) { return sub8(s, s.registers.A); });

  createOpCode(0x3C, "INC A",
               [](ProcessorState &s) { return inc8(s, s.registers.A); });
  createOpCode(0x3D, "DEC A",
               [](ProcessorState &s) { return dec8(s, s.registers.A); });

  // SBC A, r -> 0x98 - 0x9F
  createOpCode(0x98, "SBC A, B",
               [](ProcessorState &s) { return sbc8(s, s.registers.B); });
  createOpCode(0x99, "SBC A, C",
               [](ProcessorState &s) { return sbc8(s, s.registers.C); });
  createOpCode(0x9A, "SBC A, D",
               [](ProcessorState &s) { return sbc8(s, s.registers.D); });
  createOpCode(0x9B, "SBC A, E",
               [](ProcessorState &s) { return sbc8(s, s.registers.E); });
  createOpCode(0x9C, "SBC A, H",
               [](ProcessorState &s) { return sbc8(s, s.registers.H); });
  createOpCode(0x9D, "SBC A, L",
               [](ProcessorState &s) { return sbc8(s, s.registers.L); });
  createOpCode(0x9E, "SBC A, (HL)", [](ProcessorState &s) {
    return sbc8(s, s.memory[s.registers.HL]);
  });
  createOpCode(0x9F, "SBC A, A",
               [](ProcessorState &s) { return sbc8(s, s.registers.A); });

  // Immediate 8-bit Arithmetic
  createOpCode(0xC6, "ADD A, n", [](ProcessorState &s) {
    byte n = s.getNextByteFromPC();
    s.incPC();
    return add8(s, n) + 3; // 7 T-states (4 base + 3 read)
  });
  createOpCode(0xCE, "ADC A, n", [](ProcessorState &s) {
    byte n = s.getNextByteFromPC();
    s.incPC();
    return adc8(s, n) + 3;
  });
  createOpCode(0xD6, "SUB n", [](ProcessorState &s) {
    byte n = s.getNextByteFromPC();
    s.incPC();
    return sub8(s, n) + 3;
  });
  createOpCode(0xDE, "SBC A, n", [](ProcessorState &s) {
    byte n = s.getNextByteFromPC();
    s.incPC();
    return sbc8(s, n) + 3;
  });
  createOpCode(0xE6, "AND n", [](ProcessorState &s) {
    byte n = s.getNextByteFromPC();
    s.incPC();
    return and8(s, n) + 3;
  });
  createOpCode(0xEE, "XOR n", [](ProcessorState &s) {
    byte n = s.getNextByteFromPC();
    s.incPC();
    return xor8(s, n) + 3;
  });
  createOpCode(0xF6, "OR n", [](ProcessorState &s) {
    byte n = s.getNextByteFromPC();
    s.incPC();
    return or8(s, n) + 3;
  });
  createOpCode(0xFE, "CP n", [](ProcessorState &s) {
    byte n = s.getNextByteFromPC();
    s.incPC();
    return cp8(s, n) + 3;
  });

  // Flag/Misc Arithmetic
  createOpCode(0x27, "DAA", processDAA);
  createOpCode(0x2F, "CPL", processCPL);
  createOpCode(0x37, "SCF", processSCF);
  createOpCode(0x3F, "CCF", processCCF);

  // 16-bit Arithmetic
  createOpCode(INC_BC, "INC_BC", processINC_BC);
  createOpCode(INC_DE, "INC_DE", processINC_DE);
  createOpCode(INC_HL, "INC_HL", processINC_HL);
  createOpCode(INC_SP, "INC_SP", processINC_SP);

  createOpCode(INC_B, "INC_B", processINC_B);
  createOpCode(INC_C, "INC_C", processINC_C);
  createOpCode(INC_D, "INC_D", processINC_D);
  createOpCode(INC_E, "INC_E", processINC_E);
  createOpCode(INC_H, "INC_H", processINC_H);
  createOpCode(INC_L, "INC_L", processINC_L);
  createOpCode(INC_HL_REF, "INC_HL_REF", processINC_HL_REF);

  createOpCode(DEC_BC, "DEC_BC", processDEC_BC);
  createOpCode(DEC_DE, "DEC_DE", processDEC_DE);
  createOpCode(DEC_HL, "DEC_HL", processDEC_HL);
  createOpCode(DEC_SP, "DEC_SP", processDEC_SP);

  createOpCode(DEC_B, "DEC_B", processDEC_B);
  createOpCode(DEC_C, "DEC_C", processDEC_C);
  createOpCode(DEC_D, "DEC_D", processDEC_D);
  createOpCode(DEC_E, "DEC_E", processDEC_E);
  createOpCode(DEC_H, "DEC_H", processDEC_H);
  createOpCode(DEC_L, "DEC_L", processDEC_L);
  createOpCode(DEC_HL_REF, "DEC_HL_REF", processDEC_HL_REF);
}

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

  // TODO: H and P/V flags

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

  // TODO: H and P/V flags

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

int ArithmeticOpcodes::and8(ProcessorState &state, emulator_types::byte val) {
  state.registers.A &= val;
  // Flags: S, Z, H=1, P/V, N=0, C=0
  byte res = state.registers.A;
  if (res & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
  if (res == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  SET_FLAG(H_FLAG, state.registers);
  // P/V is Parity
  // TODO: Parity calc
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(C_FLAG, state.registers);
  return 4;
}

int ArithmeticOpcodes::xor8(ProcessorState &state, emulator_types::byte val) {
  state.registers.A ^= val;
  // Flags: S, Z, H=0, P/V (Parity), N=0, C=0
  byte res = state.registers.A;
  if (res & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
  if (res == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  // TODO: Parity
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(C_FLAG, state.registers);
  return 4;
}

int ArithmeticOpcodes::or8(ProcessorState &state, emulator_types::byte val) {
  state.registers.A |= val;
  // Flags: S, Z, H=0, P/V (Parity), N=0, C=0
  byte res = state.registers.A;
  if (res & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
  if (res == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  // TODO: Parity
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(C_FLAG, state.registers);
  return 4;
}

int ArithmeticOpcodes::cp8(ProcessorState &state, emulator_types::byte val) {
  // CP is like SUB but result not stored
  int res = state.registers.A - val;

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

  // H flag
  if ((state.registers.A & 0x0F) < (val & 0x0F))
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  // P/V (Overflow)
  int op1 = (int8_t)state.registers.A;
  int op2 = (int8_t)val;
  int r = (int8_t)(res & 0xFF);
  if (((op1 > 0 && op2 < 0) && r < 0) || ((op1 < 0 && op2 > 0) && r > 0)) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  SET_FLAG(N_FLAG, state.registers);
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
