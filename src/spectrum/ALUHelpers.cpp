#include "ALUHelpers.h"

namespace ALUHelpers {

void add8(ProcessorState &state, emulator_types::byte val) {
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
  if (((op1 > 0 && op2 > 0) && r < 0) || ((op1 < 0 && op2 < 0) && r > 0)) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  CLEAR_FLAG(N_FLAG, state.registers);
  state.registers.A = (emulator_types::byte)res;
}

void sub8(ProcessorState &state, emulator_types::byte val) {
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

  // Overflow
  int op1 = (int8_t)state.registers.A;
  int op2 = (int8_t)val;
  int r = (int8_t)(res & 0xFF);
  if (((op1 > 0 && op2 < 0) && r < 0) || ((op1 < 0 && op2 > 0) && r > 0)) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  SET_FLAG(N_FLAG, state.registers);
  state.registers.A = (emulator_types::byte)res;
}

void adc8(ProcessorState &state, emulator_types::byte val) {
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
  if (((op1 > 0 && op2 > 0) && r < 0) || ((op1 < 0 && op2 < 0) && r > 0)) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  CLEAR_FLAG(N_FLAG, state.registers);
  state.registers.A = (emulator_types::byte)res;
}

void sbc8(ProcessorState &state, emulator_types::byte val) {
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

  if (res < 0) // Adjusted check for borrow including carry
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);

  if (((state.registers.A & 0x0F) - (val & 0x0F) - carry) < 0)
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  // Overflow
  int op1 = (int8_t)state.registers.A;
  int op2 = (int8_t)val;
  int r = (int8_t)(res & 0xFF);
  if (((op1 > 0 && op2 < 0) && r < 0) || ((op1 < 0 && op2 > 0) && r > 0)) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  SET_FLAG(N_FLAG, state.registers);
  state.registers.A = (emulator_types::byte)res;
}

void inc8(ProcessorState &state, emulator_types::byte &reg) {
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

void dec8(ProcessorState &state, emulator_types::byte &reg) {
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

void and8(ProcessorState &state, emulator_types::byte val) {
  state.registers.A &= val;
  // Flags: S, Z, H=1, P/V=Parity, N=0, C=0
  if (state.registers.A & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
  if (state.registers.A == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  SET_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(C_FLAG, state.registers);

  // Parity
  int bits = 0;
  for (int i = 0; i < 8; i++) {
    if (state.registers.A & (1 << i))
      bits++;
  }
  if (bits % 2 == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

void or8(ProcessorState &state, emulator_types::byte val) {
  state.registers.A |= val;
  // Flags: S, Z, H=0, P/V=Parity, N=0, C=0
  if (state.registers.A & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
  if (state.registers.A == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(C_FLAG, state.registers);

  // Parity
  int bits = 0;
  for (int i = 0; i < 8; i++) {
    if (state.registers.A & (1 << i))
      bits++;
  }
  if (bits % 2 == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

void xor8(ProcessorState &state, emulator_types::byte val) {
  state.registers.A ^= val;
  // Flags: S, Z, H=0, P/V=Parity, N=0, C=0
  if (state.registers.A & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
  if (state.registers.A == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(C_FLAG, state.registers);

  // Parity
  int bits = 0;
  for (int i = 0; i < 8; i++) {
    if (state.registers.A & (1 << i))
      bits++;
  }
  if (bits % 2 == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

void cp8(ProcessorState &state, emulator_types::byte val) {
  // CP is effectively SUB but result not stored in A
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

  // Half Borrow
  if ((state.registers.A & 0x0F) < (val & 0x0F))
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  // Overflow
  int op1 = (int8_t)state.registers.A;
  int op2 = (int8_t)val;
  int r = (int8_t)(res & 0xFF);
  if (((op1 > 0 && op2 < 0) && r < 0) || ((op1 < 0 && op2 > 0) && r > 0)) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  SET_FLAG(N_FLAG, state.registers);
}

// 16-bit
int add16(ProcessorState &state, emulator_types::word &dest,
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

int inc16(ProcessorState &state, emulator_types::word &reg) {
  reg++;
  return 6;
}

int dec16(ProcessorState &state, emulator_types::word &reg) {
  reg--;
  return 6;
}

} // namespace ALUHelpers
