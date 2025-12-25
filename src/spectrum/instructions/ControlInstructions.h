#ifndef ZXEMULATOR_CONTROL_INSTRUCTIONS_H
#define ZXEMULATOR_CONTROL_INSTRUCTIONS_H

#include "../../utils/BaseTypes.h"
#include "../ProcessorMacros.h"
#include "../ProcessorState.h"
#include "LoadInstructions.h"
#include <cstdint>

namespace Control {

// Relative Jumps
// JR e (2 bytes). PC points to e.
// Logic: PC++. Return PC = Next Op. Taken: PC += e.
// e is signed.
inline int jr(ProcessorState &state, int8_t offset) {
  state.registers.PC++;
  state.registers.PC += offset;
  return 12;
}

inline int jr_cond(ProcessorState &state, bool condition, int8_t offset) {
  state.registers.PC++; // Advance past offset
  if (condition) {
    state.registers.PC += offset;
    return 12; // Taken
  }
  return 7; // Not taken
}

inline int djnz(ProcessorState &state, int8_t offset) {
  state.registers.PC++; // Advance past offset
  state.registers.B--;
  if (state.registers.B != 0) {
    state.registers.PC += offset;
    return 13;
  }
  return 8;
}

// Absolute Jumps
// JP nn (3 bytes). PC points to nn.
inline int jp(ProcessorState &state, emulator_types::word nn) {
  // PC is overwritten, no need to advance.
  state.registers.PC = nn;
  return 10;
}

inline int jp_cond(ProcessorState &state, bool condition,
                   emulator_types::word nn) {
  if (condition) {
    state.registers.PC = nn;
    return 10;
  }
  // Not taken: Skip operand (2 bytes)
  state.registers.PC += 2;
  return 10;
}

inline int jp_hl(ProcessorState &state) {
  state.registers.PC = state.registers.HL;
  return 4;
}

inline int jp_ix(ProcessorState &state) {
  state.registers.PC = state.registers.IX;
  return 8;
}

inline int jp_iy(ProcessorState &state) {
  state.registers.PC = state.registers.IY;
  return 8;
}

// Call / Return
// CALL nn (3 bytes). PC points to nn.
inline int call(ProcessorState &state, emulator_types::word nn) {
  state.registers.PC += 2; // Advance past operand (Return Address)
  Load::push16(state, state.registers.PC);
  state.registers.PC = nn;
  return 17;
}

inline int call_cond(ProcessorState &state, bool condition,
                     emulator_types::word nn) {
  state.registers.PC += 2; // Advance past operand
  if (condition) {
    Load::push16(state, state.registers.PC);
    state.registers.PC = nn;
    return 17;
  }
  return 10;
}

inline int ret(ProcessorState &state) {
  state.registers.PC = Load::pop16(state);
  return 10;
}

inline int ret_cond(ProcessorState &state, bool condition) {
  if (condition) {
    state.registers.PC = Load::pop16(state);
    return 11;
  }
  return 5;
}

inline int rst(ProcessorState &state, emulator_types::word address) {
  Load::push16(state, state.registers.PC); // PC is already next op
  state.registers.PC = address;
  return 11;
}

// Search (Block)
inline int cpi(ProcessorState &state) {
  // Compare A with (HL), HL++, BC--
  emulator_types::byte value = state.memory[state.registers.HL];
  int result = state.registers.A - value;

  bool z = (result == 0);
  if (z)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  SET_FLAG(N_FLAG, state.registers); // CP sets N

  // S Flag
  if (result & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // H Flag
  if ((state.registers.A & 0x0F) < (value & 0x0F))
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  state.registers.HL++;
  state.registers.BC--;

  bool bcNonZero = (state.registers.BC != 0);
  if (bcNonZero)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers); // P/V indicates BC!=0

  return 16;
}

inline int cpd(ProcessorState &state) {
  // Compare A with (HL), HL--, BC--
  emulator_types::byte value = state.memory[state.registers.HL];
  int result = state.registers.A - value;

  bool z = (result == 0);
  if (z)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  SET_FLAG(N_FLAG, state.registers); // CP sets N

  // S Flag
  if (result & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // H Flag
  if ((state.registers.A & 0x0F) < (value & 0x0F))
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  state.registers.HL--;
  state.registers.BC--;

  bool bcNonZero = (state.registers.BC != 0);
  if (bcNonZero)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers); // P/V indicates BC!=0

  return 16;
}

inline int cpir(ProcessorState &state) {
  // Compare A with (HL), HL++, BC--
  emulator_types::byte value = state.memory[state.registers.HL];
  int result = state.registers.A - value;

  bool z = (result == 0);
  if (z)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  SET_FLAG(N_FLAG, state.registers); // CP sets N

  // S Flag
  if (result & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // H Flag
  if ((state.registers.A & 0x0F) < (value & 0x0F))
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  state.registers.HL++;
  state.registers.BC--;

  bool bcNonZero = (state.registers.BC != 0);
  if (bcNonZero)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers); // P/V indicates BC!=0

  // If BC!=0 AND !Z, repeat
  if (bcNonZero && !z) {
    state.registers.PC -= 2;
    return 21;
  } else {
    return 16;
  }
}

inline int cpdr(ProcessorState &state) {
  // Compare A with (HL), HL--, BC--
  emulator_types::byte value = state.memory[state.registers.HL];
  int result = state.registers.A - value;

  bool z = (result == 0);
  if (z)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  SET_FLAG(N_FLAG, state.registers); // CP sets N

  // S Flag
  if (result & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // H Flag
  if ((state.registers.A & 0x0F) < (value & 0x0F))
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  state.registers.HL--;
  state.registers.BC--;

  bool bcNonZero = (state.registers.BC != 0);
  if (bcNonZero)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers); // P/V indicates BC!=0

  // If BC!=0 AND !Z, repeat
  if (bcNonZero && !z) {
    state.registers.PC -= 2;
    return 21;
  } else {
    return 16;
  }
}

inline int reti(ProcessorState &state) {
  state.registers.PC = Load::pop16(state);
  return 14;
}

inline int retn(ProcessorState &state) {
  state.registers.PC = Load::pop16(state);
  state.registers.IFF1 = state.registers.IFF2;
  state.setInterrupts(state.registers.IFF1);
  return 14;
}

} // namespace Control

#endif
