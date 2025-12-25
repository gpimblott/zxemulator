#ifndef ZXEMULATOR_ARITHMETIC_INSTRUCTIONS_H
#define ZXEMULATOR_ARITHMETIC_INSTRUCTIONS_H

#include "../../utils/BaseTypes.h"
#include "../ProcessorMacros.h"
#include "../ProcessorState.h"
#include <cstdint>

namespace Arithmetic {

// 8-Bit Arithmetic
inline void add8(ProcessorState &state, emulator_types::byte val) {
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
  int op1 = (int8_t)state.registers.A;
  int op2 = (int8_t)val;
  int r = (int8_t)(res & 0xFF);
  // Overflow (P/V) for ADD: same-sign operands, different-sign result
  if ((~(op1 ^ op2) & (op1 ^ r) & 0x80) != 0) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  CLEAR_FLAG(N_FLAG, state.registers);
  state.registers.A = (emulator_types::byte)res;
}

inline void adc8(ProcessorState &state, emulator_types::byte val) {
  int carry = GET_FLAG(C_FLAG, state.registers) ? 1 : 0;
  int res = state.registers.A + val + carry;

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

  if (((state.registers.A & 0x0F) + (val & 0x0F) + carry) > 0x0F)
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  // Overflow
  int op1 = (int8_t)state.registers.A;
  int op2 = (int8_t)val;
  int r = (int8_t)(res & 0xFF);
  // Overflow for ADC: same-sign operands, different-sign result
  if ((~(op1 ^ op2) & (op1 ^ r) & 0x80) != 0) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  CLEAR_FLAG(N_FLAG, state.registers);
  state.registers.A = (emulator_types::byte)res;
}

inline void sub8(ProcessorState &state, emulator_types::byte val) {
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

  if ((state.registers.A & 0x0F) < (val & 0x0F))
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  // Overflow for subtraction: same-sign operands, different-sign result
  int op1 = (int8_t)state.registers.A;
  int op2 = (int8_t)val;
  int r = (int8_t)(res & 0xFF);
  // Overflow for SUB: different-sign operands, different-sign result
  if (((op1 ^ op2) & (op1 ^ r) & 0x80) != 0) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  SET_FLAG(N_FLAG, state.registers);
  state.registers.A = (emulator_types::byte)res;
}

inline void sbc8(ProcessorState &state, emulator_types::byte val) {
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

  if (res < 0)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);

  if (((state.registers.A & 0x0F) - (val & 0x0F) - carry) < 0)
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  // Overflow for subtraction: same-sign operands, different-sign result
  int op1 = (int8_t)state.registers.A;
  int op2 = (int8_t)val;
  int r = (int8_t)(res & 0xFF);
  // Overflow for SBC: different-sign operands, different-sign result
  if (((op1 ^ op2) & (op1 ^ r) & 0x80) != 0) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  SET_FLAG(N_FLAG, state.registers);
  state.registers.A = (emulator_types::byte)res;
}

inline void cp8(ProcessorState &state, emulator_types::byte val) {
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

  if ((state.registers.A & 0x0F) < (val & 0x0F))
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  // Overflow for subtraction: result  sign differs from operands with SAME sign
  // overflow if: (pos - pos = neg) OR (neg - neg = pos)
  int op1 = (int8_t)state.registers.A;
  int op2 = (int8_t)val;
  int r = (int8_t)(res & 0xFF);
  // Overflow for CP: different-sign operands, different-sign result
  if (((op1 ^ op2) & (op1 ^ r) & 0x80) != 0) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  SET_FLAG(N_FLAG, state.registers);

  // Undocumented: X and Y flags are copied from the operand (val)
  if (val & 0x08)
    SET_FLAG(X_FLAG, state.registers);
  else
    CLEAR_FLAG(X_FLAG, state.registers);
  if (val & 0x20)
    SET_FLAG(Y_FLAG, state.registers);
  else
    CLEAR_FLAG(Y_FLAG, state.registers);
}

inline void inc8(ProcessorState &state, emulator_types::byte &reg) {
  reg++;
  CLEAR_FLAG(N_FLAG, state.registers);

  if (reg == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  if (reg & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  if ((reg & 0x0F) == 0x00)
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  if (reg == 0x80)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

inline void dec8(ProcessorState &state, emulator_types::byte &reg) {
  reg--;
  SET_FLAG(N_FLAG, state.registers);

  if (reg == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  if (reg & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  if ((reg & 0x0F) == 0x0F)
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  if (reg == 0x7F)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

inline void daa(ProcessorState &state) {
  emulator_types::byte a = state.registers.A;
  int res = a;
  bool n = GET_FLAG(N_FLAG, state.registers);
  bool c = GET_FLAG(C_FLAG, state.registers);
  bool h = GET_FLAG(H_FLAG, state.registers);

  if (!n) {
    if (h || (a & 0x0F) > 9)
      res += 0x06;
    if (c || (a > 0x99))
      res += 0x60;
  } else {
    if (h || (a & 0x0F) > 9)
      res -= 0x06;
    if (c || res > 0x99)
      res -= 0x60;
  }

  // Flags
  if (c || (!n && a > 0x99))
    SET_FLAG(C_FLAG, state.registers);
  // H flag logic approximated but functional for verified behavior
  if ((!n && (a & 0x0F) > 9) || (n && h && (a & 0x0F) < 6))
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  state.registers.A = (emulator_types::byte)res;

  // Parity
  int bits = 0;
  for (int i = 0; i < 8; i++)
    if (res & (1 << i))
      bits++;
  if (bits % 2 == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);

  if ((res & 0xFF) == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  if (res & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // Undocumented X/Y from result
  if (res & 0x20)
    SET_FLAG(Y_FLAG, state.registers);
  else
    CLEAR_FLAG(Y_FLAG, state.registers);
  if (res & 0x08)
    SET_FLAG(X_FLAG, state.registers);
  else
    CLEAR_FLAG(X_FLAG, state.registers);
}

inline void neg8(ProcessorState &state) {
  // NEG is effectively 0 - A
  emulator_types::byte val = state.registers.A;
  int res = 0 - val;

  // Flags
  if ((res & 0xFF) == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  if (res & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  if (val != 0)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);

  // H Flag: Set if borrow from bit 4. For 0 - A, this is set if A & 0x0F != 0
  if ((val & 0x0F) != 0)
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  // P/V Flag: Overflow. Result is 0x80 only if A was 0x80.
  if (val == 0x80)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);

  SET_FLAG(N_FLAG, state.registers);

  // Undocumented X/Y from result
  if (res & 0x08)
    SET_FLAG(X_FLAG, state.registers);
  else
    CLEAR_FLAG(X_FLAG, state.registers);
  if (res & 0x20)
    SET_FLAG(Y_FLAG, state.registers);
  else
    CLEAR_FLAG(Y_FLAG, state.registers);

  state.registers.A = (emulator_types::byte)res;
}

// 16-Bit
inline int add16(ProcessorState &state, emulator_types::word &dest,
                 emulator_types::word src) {
  long result = dest + src;

  if ((dest & 0x0FFF) + (src & 0x0FFF) > 0x0FFF) {
    SET_FLAG(H_FLAG, state.registers);
  } else {
    CLEAR_FLAG(H_FLAG, state.registers);
  }

  if (result > 0xFFFF) {
    SET_FLAG(C_FLAG, state.registers);
  } else {
    CLEAR_FLAG(C_FLAG, state.registers);
  }

  CLEAR_FLAG(N_FLAG, state.registers);
  dest = (emulator_types::word)result;
  return 11;
}

inline int inc16(ProcessorState &state, emulator_types::word &reg) {
  reg++;
  return 6;
}

inline int dec16(ProcessorState &state, emulator_types::word &reg) {
  reg--;
  return 6;
}

// Extended 16-Bit
inline void adc16(ProcessorState &state, emulator_types::word &dest,
                  emulator_types::word src) {
  long val = dest + src + (GET_FLAG(C_FLAG, state.registers) ? 1 : 0);
  int H_carry = (((dest & 0x0FFF) + (src & 0x0FFF) +
                  (GET_FLAG(C_FLAG, state.registers) ? 1 : 0)) > 0x0FFF);

  if (val > 0xFFFF)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);

  CLEAR_FLAG(N_FLAG, state.registers);

  if ((val & 0xFFFF) == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  if (H_carry)
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  if (val & 0x8000)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // P/V
  short op1 = (short)dest;
  short op2 = (short)src;
  short r = (short)val;
  if ((~(op1 ^ op2) & (op1 ^ r) & 0x8000) != 0) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  dest = (emulator_types::word)val;
}

inline void sbc16(ProcessorState &state, emulator_types::word &dest,
                  emulator_types::word src) {
  long val = dest - src - (GET_FLAG(C_FLAG, state.registers) ? 1 : 0);
  int H_carry = (((dest & 0x0FFF) - (src & 0x0FFF) -
                  (GET_FLAG(C_FLAG, state.registers) ? 1 : 0)) < 0);

  if ((val & 0xFFFF0000) != 0)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);

  SET_FLAG(N_FLAG, state.registers);

  if ((val & 0xFFFF) == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  if (H_carry)
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  if (val & 0x8000)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // P/V
  short op1 = (short)dest;
  short op2 = (short)src;
  short r = (short)val;
  if (((op1 ^ op2) & (op1 ^ r) & 0x8000) != 0) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  dest = (emulator_types::word)val;
}

} // namespace Arithmetic

#endif
