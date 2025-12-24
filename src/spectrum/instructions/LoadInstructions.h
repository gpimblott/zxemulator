#ifndef ZXEMULATOR_LOAD_INSTRUCTIONS_H
#define ZXEMULATOR_LOAD_INSTRUCTIONS_H

#include "../../utils/BaseTypes.h"
#include "../ProcessorMacros.h"
#include "../ProcessorState.h"

namespace Load {

// 16-bit PUSH/POP
inline void push16(ProcessorState &state, emulator_types::word value) {
  state.registers.SP -= 2;
  state.memory.fastWrite(state.registers.SP + 1, (value >> 8) & 0xFF);
  state.memory.fastWrite(state.registers.SP, value & 0xFF);
}

inline emulator_types::word pop16(ProcessorState &state) {
  emulator_types::byte low = state.memory[state.registers.SP];
  emulator_types::byte high = state.memory[state.registers.SP + 1];
  state.registers.SP += 2;
  return (high << 8) | low;
}

// Extended Loads
inline void ld_nn_rr(ProcessorState &state, emulator_types::word nn,
                     emulator_types::word rr) {
  state.memory.fastWrite(nn, (emulator_types::byte)(rr & 0xFF));
  state.memory.fastWrite(nn + 1, (emulator_types::byte)((rr >> 8) & 0xFF));
}

inline void ld_rr_nn(ProcessorState &state, emulator_types::word &rr,
                     emulator_types::word nn) {
  emulator_types::byte low = state.memory[nn];
  emulator_types::byte high = state.memory[nn + 1];
  rr = (emulator_types::word)(high << 8) | low;
}

// Block Transfer (LDIR/LDDR)
// Note: These helpers perform ONE STEP. The calling loop in Processor handles
// PC -= 2. Wait, Processor::op_ed_ldir handles the loop internally returns
// cycle count. I should duplicate that logic: Return cycle count and manage PC?
// Yes.

inline int ldir(ProcessorState &state) {
  emulator_types::byte value = state.memory[state.registers.HL];
  state.memory.fastWrite(state.registers.DE, value);
  state.registers.DE++;
  state.registers.HL++;
  state.registers.BC--;

  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(P_FLAG, state.registers);

  if (state.registers.BC != 0) {
    state.registers.PC -= 2;
    return 21;
  } else {
    return 16;
  }
}

inline int lddr(ProcessorState &state) {
  emulator_types::byte value = state.memory[state.registers.HL];
  state.memory.fastWrite(state.registers.DE, value);
  state.registers.DE--;
  state.registers.HL--;
  state.registers.BC--;

  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(P_FLAG, state.registers);

  if (state.registers.BC != 0) {
    state.registers.PC -= 2;
    return 21;
  } else {
    return 16;
  }
}

// Single step versions (LDI/LDD) - If needed.
// Processor.cpp had LDIR/LDDR but apparently not LDI/LDD logic explicitly
// (unless shared). I will implement LDI/LDD logic cleanly if I use them. LDI:
// Transfer, Inc, Dec BC. No loop.
inline int ldi(ProcessorState &state) {
  emulator_types::byte value = state.memory[state.registers.HL];
  state.memory.fastWrite(state.registers.DE, value);
  state.registers.DE++;
  state.registers.HL++;
  state.registers.BC--;

  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);

  if (state.registers.BC != 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);

  return 16;
}

inline int ldd(ProcessorState &state) {
  emulator_types::byte value = state.memory[state.registers.HL];
  state.memory.fastWrite(state.registers.DE, value);
  state.registers.DE--;
  state.registers.HL--;
  state.registers.BC--;

  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);

  if (state.registers.BC != 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);

  return 16;
}

} // namespace Load

#endif
