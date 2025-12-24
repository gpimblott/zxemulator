#ifndef ZXEMULATOR_LOGIC_INSTRUCTIONS_H
#define ZXEMULATOR_LOGIC_INSTRUCTIONS_H

#include "../../utils/BaseTypes.h"
#include "../ProcessorMacros.h"
#include "../ProcessorState.h"

namespace Logic {

inline void and8(ProcessorState &state, emulator_types::byte val) {
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

  SET_FLAG(H_FLAG, state.registers); // H is set for AND
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

inline void or8(ProcessorState &state, emulator_types::byte val) {
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

  CLEAR_FLAG(H_FLAG, state.registers); // H is reset for OR
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

inline void xor8(ProcessorState &state, emulator_types::byte val) {
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

  CLEAR_FLAG(H_FLAG, state.registers); // H is reset for XOR
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

} // namespace Logic

#endif
