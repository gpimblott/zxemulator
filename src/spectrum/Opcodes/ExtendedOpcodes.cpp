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

#include "ExtendedOpcodes.h"
#include "../../utils/debug.h"

ExtendedOpcodes::ExtendedOpcodes() : OpCodeProvider() {
  createOpCode(0xED, "EXTENDED", processExtended);
}

int ExtendedOpcodes::processLDIR(ProcessorState &state) {
  // (DE)<-(HL), DE++, HL++, BC--
  byte value = state.memory[state.registers.HL];
  state.memory[state.registers.DE] = value;
  state.registers.DE++;
  state.registers.HL++;
  state.registers.BC--;
  // Reset PV flag, reset N, H
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(P_FLAG, state.registers); // P/V reset to 0

  // If BC!=0 repeat
  if (state.registers.BC != 0) {
    state.decPC(2); // Back 2 bytes (ED B0)
    return 21;
  }
  return 16;
}

int ExtendedOpcodes::processLDDR(ProcessorState &state) {
  // (DE)<-(HL), DE--, HL--, BC--
  byte value = state.memory[state.registers.HL];
  state.memory[state.registers.DE] = value;
  state.registers.DE--;
  state.registers.HL--;
  state.registers.BC--;

  // Reset PV flag, reset N, H
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(P_FLAG, state.registers);

  // If BC!=0 repeat
  if (state.registers.BC != 0) {
    state.decPC(2); // Back 2 bytes (ED B8)
    return 21;
  }
  return 16;
}

int ExtendedOpcodes::processExtended(ProcessorState &state) {
  byte extOpcode = state.getNextByteFromPC();
  state.incPC(); // Consume the extended opcode byte

  switch (extOpcode) {
  case 0x47: // LD I, A
    state.registers.I = state.registers.A;
    // debug("LD I, A\n");
    state.registers.I = state.registers.A;
    // debug("LD I, A\n");
    return 9;
  case 0x78: // IN A, (C)
  {
    // Input from port BC
    // The port address is in BC.
    // Spectrum ULA responds to all even ports (bit 0 = 0).
    int port = state.registers.BC;
    if ((port & 1) == 0) {
      state.registers.A = state.keyboard.readPort((port >> 8) & 0xFF);
    } else {
      state.registers.A = 0xFF;
    }

    // Flags: S, Z, H, P/V, N affected. C preserved.
    // Preserve Carry
    bool carry = GET_FLAG(C_FLAG, state.registers);

    // Clear all except C? No, standard behavior is specific updates.
    // But usually we build F from scratch or modify.
    // Let's reset F but keep C.
    state.registers.F = 0;
    if (carry)
      SET_FLAG(C_FLAG, state.registers);

    // Set result flags based on result
    if (state.registers.A & 0x80)
      SET_FLAG(S_FLAG, state.registers);
    else
      CLEAR_FLAG(S_FLAG, state.registers);

    if (state.registers.A == 0)
      SET_FLAG(Z_FLAG, state.registers);
    else
      CLEAR_FLAG(Z_FLAG, state.registers);

    CLEAR_FLAG(H_FLAG, state.registers);

    // Parity
    int parity = 0;
    for (int i = 0; i < 8; i++) {
      if ((state.registers.A >> i) & 1)
        parity++;
    }
    if (parity % 2 == 0)
      SET_FLAG(P_FLAG, state.registers); // P/V Set if Parity Even
    else
      CLEAR_FLAG(P_FLAG, state.registers);

    CLEAR_FLAG(N_FLAG, state.registers);

    return 12;
  }

  case 0x4F: // LD R, A
    state.registers.R = state.registers.A;
    // debug("LD R, A\n");
    return 9;
  case 0x56: // IM 1
    // TODO: Set Interrupt Mode 1
    // debug("IM 1\n");
    return 8;
  case 0x5E: // IM 2
    // TODO: Set Interrupt Mode 2
    // debug("IM 2\n");
    return 8;

  // SBC HL, rr
  case 0x42: // SBC HL, BC
    return sbc16(state, state.registers.BC);
  case 0x52: // SBC HL, DE
    return sbc16(state, state.registers.DE);
  case 0x62: // SBC HL, HL
    return sbc16(state, state.registers.HL);
  case 0x72: // SBC HL, SP
    return sbc16(state, state.registers.SP);

  // ADC HL, rr
  case 0x4A: // ADC HL, BC
    return adc16(state, state.registers.BC);
  case 0x5A: // ADC HL, DE
    return adc16(state, state.registers.DE);
  case 0x6A: // ADC HL, HL
    return adc16(state, state.registers.HL);
  case 0x7A: // ADC HL, SP
    return adc16(state, state.registers.SP);

  // LD (nn), rr
  case 0x43: // LD (nn), BC
    return ld_nn_rr(state, state.registers.BC);
  case 0x53: // LD (nn), DE
    return ld_nn_rr(state, state.registers.DE);
  case 0x63: // LD (nn), HL (duplicates 0x22)
    return ld_nn_rr(state, state.registers.HL);
  case 0x73: // LD (nn), SP
    return ld_nn_rr(state, state.registers.SP);

  // LD rr, (nn)
  case 0x4B: // LD BC, (nn)
    return ld_rr_nn(state, state.registers.BC);
  case 0x5B: // LD DE, (nn)
    return ld_rr_nn(state, state.registers.DE);
  case 0x6B: // LD HL, (nn) (duplicates 0x2A)
    return ld_rr_nn(state, state.registers.HL);
  case 0x7B: // LD SP, (nn)
    return ld_rr_nn(state, state.registers.SP);

  case 0xB0: // LDIR
    return processLDIR(state);
  case 0xB8: // LDDR
    return processLDDR(state);
  case 0xB1: // CPIR
    return processCPIR(state);
  default:
    // debug("Unknown Extended Opcode %02X\n", extOpcode);
    return 0;
  }
}

int ExtendedOpcodes::processCPIR(ProcessorState &state) {
  // Compare A with (HL), HL++, BC--
  // Repeat if A!=(HL) and BC!=0
  word HL = state.registers.HL;
  word BC = state.registers.BC;
  byte val = state.memory[HL];
  byte A = state.registers.A;

  // Flags... skipping complex CMP flags for now, just main functionality
  bool equal = (A == val);

  state.registers.HL = HL + 1;
  state.registers.BC = BC - 1;

  // debug("CPIR (BC=%d)\n", state.registers.BC);

  if (!equal && state.registers.BC != 0) {
    state.decPC(2); // Repeat instruction (ED B1)
    return 21;
  }
  return 16;
}

int ExtendedOpcodes::sbc16(ProcessorState &state, emulator_types::word val) {
  int op1 = state.registers.HL;
  int op2 = val;
  int carry = GET_FLAG(C_FLAG, state.registers) ? 1 : 0;
  int result = op1 - op2 - carry;

  SET_FLAG(N_FLAG, state.registers);

  if (result < 0)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);

  if ((result & 0xFFFF) == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  if (result & 0x8000)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  state.registers.HL = (word)result;
  return 15;
}

int ExtendedOpcodes::adc16(ProcessorState &state, emulator_types::word val) {
  int op1 = state.registers.HL;
  int op2 = val;
  int carry = GET_FLAG(C_FLAG, state.registers) ? 1 : 0;
  int result = op1 + op2 + carry;

  CLEAR_FLAG(N_FLAG, state.registers);

  if (result > 0xFFFF)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);

  if ((result & 0xFFFF) == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  if (result & 0x8000)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  state.registers.HL = (word)result;
  return 15;
}

int ExtendedOpcodes::ld_nn_rr(ProcessorState &state, emulator_types::word val) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();

  word address = (high << 8) | low;

  state.memory[address] = (byte)(val & 0xFF);
  state.memory[address + 1] = (byte)((val >> 8) & 0xFF);

  return 20;
}

int ExtendedOpcodes::ld_rr_nn(ProcessorState &state,
                              emulator_types::word &reg) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();

  word address = (high << 8) | low;

  byte valL = state.memory[address];
  byte valH = state.memory[address + 1];

  reg = (valH << 8) | valL;

  return 20;
}
