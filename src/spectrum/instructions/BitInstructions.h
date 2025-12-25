#ifndef ZXEMULATOR_BIT_INSTRUCTIONS_H
#define ZXEMULATOR_BIT_INSTRUCTIONS_H

#include "../../utils/BaseTypes.h"
#include "../ProcessorMacros.h"
#include "../ProcessorState.h"
#include <cstdint>

namespace Bit {

inline void rlc(ProcessorState &state, emulator_types::byte &val) {
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
  int bits = 0;
  for (int i = 0; i < 8; i++) {
    if (val & (1 << i))
      bits++;
  }
  if (bits % 2 == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

inline void rrc(ProcessorState &state, emulator_types::byte &val) {
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

  // P/V Parity
  int bits = 0;
  for (int i = 0; i < 8; i++) {
    if (val & (1 << i))
      bits++;
  }
  if (bits % 2 == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

inline void rl(ProcessorState &state, emulator_types::byte &val) {
  int carry = (val & 0x80) ? 1 : 0;
  int oldCarry = GET_FLAG(C_FLAG, state.registers) ? 1 : 0;
  val = (val << 1) | oldCarry;

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
  int bits = 0;
  for (int i = 0; i < 8; i++) {
    if (val & (1 << i))
      bits++;
  }
  if (bits % 2 == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

inline void rr(ProcessorState &state, emulator_types::byte &val) {
  int carry = (val & 0x01) ? 1 : 0;
  int oldCarry = GET_FLAG(C_FLAG, state.registers) ? 1 : 0;
  val = (val >> 1) | (oldCarry << 7);

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
  int bits = 0;
  for (int i = 0; i < 8; i++) {
    if (val & (1 << i))
      bits++;
  }
  if (bits % 2 == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

inline void sla(ProcessorState &state, emulator_types::byte &val) {
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

  // P/V Parity
  int bits = 0;
  for (int i = 0; i < 8; i++) {
    if (val & (1 << i))
      bits++;
  }
  if (bits % 2 == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

inline void sra(ProcessorState &state, emulator_types::byte &val) {
  int carry = (val & 0x01) ? 1 : 0;
  int msb = val & 0x80;
  val = (val >> 1) | msb;

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
  int bits = 0;
  for (int i = 0; i < 8; i++) {
    if (val & (1 << i))
      bits++;
  }
  if (bits % 2 == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

inline void sll(ProcessorState &state, emulator_types::byte &val) {
  // SLL (Undocumented): Shift Left Logical, inserts 1 into bit 0
  int carry = (val & 0x80) ? 1 : 0;
  val = (val << 1) | 0x01;

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
  int bits = 0;
  for (int i = 0; i < 8; i++) {
    if (val & (1 << i))
      bits++;
  }
  if (bits % 2 == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

inline void srl(ProcessorState &state, emulator_types::byte &val) {
  int carry = (val & 0x01) ? 1 : 0;
  val = val >> 1;

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
  int bits = 0;
  for (int i = 0; i < 8; i++) {
    if (val & (1 << i))
      bits++;
  }
  if (bits % 2 == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

inline void bit(ProcessorState &state, int bit, emulator_types::byte val) {
  bool z = !((val >> bit) & 1);
  if (z)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  SET_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);

  // Undocumented Flags:
  // S, 5 (Y), 3 (X) are copied from the input value
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  if (val & 0x20)
    SET_FLAG(Y_FLAG, state.registers);
  else
    CLEAR_FLAG(Y_FLAG, state.registers);

  if (val & 0x08)
    SET_FLAG(X_FLAG, state.registers);
  else
    CLEAR_FLAG(X_FLAG, state.registers);

  // P/V is set to the same state as Z
  if (z)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

inline void set(ProcessorState &state, int bit, emulator_types::byte &val) {
  val |= (1 << bit);
}

inline void res(ProcessorState &state, int bit, emulator_types::byte &val) {
  val &= ~(1 << bit);
}

// Rotate Decimal
inline void rrd(ProcessorState &state) {
  emulator_types::byte a = state.registers.A;
  emulator_types::byte hl = state.memory[state.registers.HL];

  emulator_types::byte finalA = (a & 0xF0) | (hl & 0x0F);
  emulator_types::byte finalHL = ((a & 0x0F) << 4) | ((hl >> 4) & 0x0F);

  state.registers.A = finalA;
  state.memory.fastWrite(state.registers.HL, finalHL);

  if (finalA & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
  if (finalA == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);

  int p = 0;
  for (int i = 0; i < 8; i++)
    if (finalA & (1 << i))
      p++;
  if ((p % 2) == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

inline void rld(ProcessorState &state) {
  emulator_types::byte a = state.registers.A;
  emulator_types::byte hl = state.memory[state.registers.HL];

  emulator_types::byte finalA = (a & 0xF0) | ((hl >> 4) & 0x0F);
  emulator_types::byte finalHL = ((hl & 0x0F) << 4) | (a & 0x0F);

  state.registers.A = finalA;
  state.memory.fastWrite(state.registers.HL, finalHL);

  if (finalA & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
  if (finalA == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);

  int p = 0;
  for (int i = 0; i < 8; i++)
    if (finalA & (1 << i))
      p++;
  if ((p % 2) == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

} // namespace Bit

#endif
