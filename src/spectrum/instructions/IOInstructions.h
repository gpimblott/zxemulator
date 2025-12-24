#ifndef ZXEMULATOR_IO_INSTRUCTIONS_H
#define ZXEMULATOR_IO_INSTRUCTIONS_H

#include "../../utils/BaseTypes.h"
#include "../ProcessorMacros.h"
#include "../ProcessorState.h"
#include "LoadInstructions.h" // For memory writes? No, Memory.fastWrite uses state.
#include <cstdint>

namespace IO {

// IN A, (n)
inline int in_a_n(ProcessorState &state, emulator_types::byte port) {
  // A8-A15 = A register. A0-A7 = n.
  emulator_types::byte high = state.registers.A;

  // Standard ULA/Keyboard read logic (Port FE)
  // If bit 0 of port is 0, read keyboard.
  if ((port & 0x01) == 0) {
    emulator_types::byte ear = state.tape.getEarBit() ? 0x40 : 0x00;
    state.registers.A = state.keyboard.readPort(high) | ear;
  } else if ((port & 0x1F) == 0x1F) {
    // Kempston Joystick (Port 31 - 0x1F)
    state.registers.A = state.keyboard.readKempstonPort();
  } else {
    // Floating bus (approximate)
    state.registers.A = 0xFF;
  }
  return 11;
}

// OUT (n), A
inline int out_n_a(ProcessorState &state, emulator_types::byte port) {
  // Port FE control.
  // Lower address = n. Upper address = A.

  if ((port & 0x01) == 0) {
    emulator_types::byte value = state.registers.A;
    emulator_types::byte borderColor = value & 0x07;
    if (state.memory.getVideoBuffer()) {
      state.memory.getVideoBuffer()->setBorderColor(borderColor,
                                                    state.getFrameTStates());
    }
    state.setSpeakerBit((value & 0x10) != 0);
    state.setMicBit((value & 0x08) != 0);
  }
  return 11;
}

// IN r, (C)
// Input from port BC to register r.
// Flags: S, Z, H=0, P/V, N=0.
inline void in_r_c(ProcessorState &state, emulator_types::byte &r) {
  // Port = BC. (B is high).
  emulator_types::byte val = 0xFF;

  // Logic from Processor::op_ed_in_r_C
  // Check ULA FE
  if ((state.registers.C & 1) == 0) {
    val = state.keyboard.readPort(state.registers.B);
    emulator_types::byte ear = state.tape.getEarBit() ? 0x40 : 0x00;
    val |= ear;
  } else {
    // Default open bus?
    val = 0xFF;
  }

  r = val;

  // Flags
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);

  // Parity
  int p = 0;
  for (int i = 0; i < 8; i++) {
    if (val & (1 << i))
      p++;
  }
  if ((p % 2) == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

// OUT (C), r
inline void out_c_r(ProcessorState &state, emulator_types::byte r) {
  emulator_types::word port = (state.registers.B << 8) | state.registers.C;
  if ((port & 0xFF) == 0xFE) { // Simple FE check
    // Use logic similar to out_n_a but value comes from r
    emulator_types::byte value = r;
    emulator_types::byte borderColor = value & 0x07;
    if (state.memory.getVideoBuffer()) {
      state.memory.getVideoBuffer()->setBorderColor(borderColor,
                                                    state.getFrameTStates());
    }
    state.setSpeakerBit((value & 0x10) != 0);
    state.setMicBit((value & 0x08) != 0);
  }
}

// Block IO
// INI: (HL) <- IN(BC), B--, HL++
inline int ini(ProcessorState &state) {
  // For block IO, we assume generic port read? Or ULA?
  // Existing Processor code used state.keyboard.readPort(B) if standard?
  // Actually usually INI uses specific port properties.
  // We'll trust state.keyboard.readPort(B) logic or simplify.

  // Reuse in_r_c logic partially?
  // But INI sets flags differently (Z based on B).

  emulator_types::byte val = 0xFF;
  // Simplified read (consistent with existing)
  // Check FE
  if ((state.registers.C & 1) == 0) {
    val = state.keyboard.readPort(state.registers.B);
    // Ear bit?
    emulator_types::byte ear = state.tape.getEarBit() ? 0x40 : 0x00;
    val |= ear;
  }

  state.memory.fastWrite(state.registers.HL, val);

  state.registers.HL++;
  state.registers.B--;

  SET_FLAG(N_FLAG, state.registers);
  if (state.registers.B == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  return 16;
}

inline int inir(ProcessorState &state) {
  ini(state);
  if (state.registers.B != 0) {
    state.registers.PC -= 2;
    return 21;
  }
  return 16;
}

inline int ind(ProcessorState &state) {
  emulator_types::byte val = 0xFF;
  if ((state.registers.C & 1) == 0) {
    val = state.keyboard.readPort(state.registers.B);
    emulator_types::byte ear = state.tape.getEarBit() ? 0x40 : 0x00;
    val |= ear;
  }

  state.memory.fastWrite(state.registers.HL, val);
  state.registers.HL--;
  state.registers.B--;

  SET_FLAG(N_FLAG, state.registers);
  if (state.registers.B == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  return 16;
}

inline int indr(ProcessorState &state) {
  ind(state);
  if (state.registers.B != 0) {
    state.registers.PC -= 2;
    return 21;
  }
  return 16;
}

// OUTI: Read (HL), B--, OUT(BC), HL++
inline int outi(ProcessorState &state) {
  emulator_types::byte val = state.memory[state.registers.HL];
  state.registers.B--;

  // Output
  out_c_r(state, val);

  state.registers.HL++;

  SET_FLAG(N_FLAG, state.registers);
  if (state.registers.B == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  return 16;
}

inline int otir(ProcessorState &state) {
  outi(state);
  if (state.registers.B != 0) {
    state.registers.PC -= 2;
    return 21;
  }
  return 16;
}

inline int outd(ProcessorState &state) {
  emulator_types::byte val = state.memory[state.registers.HL];
  state.registers.B--;

  out_c_r(state, val);

  state.registers.HL--;

  SET_FLAG(N_FLAG, state.registers);
  if (state.registers.B == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  return 16;
}

inline int otdr(ProcessorState &state) {
  outd(state);
  if (state.registers.B != 0) {
    state.registers.PC -= 2;
    return 21;
  }
  return 16;
}

} // namespace IO

#endif
