#include "IndexOpcodes.h"
#include "../../utils/debug.h"
#include "instructions/ArithmeticOpcodes.h"
#include "instructions/LogicOpcodes.h"

IndexOpcodes::IndexOpcodes() : OpCodeProvider() {
  createOpCode(0xDD, "IX Prefix", processIX);
  createOpCode(0xFD, "IY Prefix", processIY);
}

int IndexOpcodes::processIX(ProcessorState &state) {
  return processIndex(state, state.registers.IX);
}

int IndexOpcodes::processIY(ProcessorState &state) {
  return processIndex(state, state.registers.IY);
}

int IndexOpcodes::processIndex(ProcessorState &state,
                               emulator_types::word &indexReg) {
  byte opcode = state.getNextByteFromPC();
  state.incPC();

  switch (opcode) {
  case 0x21: // LD IX/IY, nn
  {
    byte low = state.getNextByteFromPC();
    state.incPC();
    byte high = state.getNextByteFromPC();
    state.incPC();
    indexReg = (high << 8) | low;
    return 14;
  }
  case 0x09: // ADD IX/IY, BC
    return ArithmeticOpcodes::add16(state, indexReg, state.registers.BC) + 4;
  case 0x19: // ADD IX/IY, DE
    return ArithmeticOpcodes::add16(state, indexReg, state.registers.DE) + 4;
  case 0x23: // INC IX/IY
    return ArithmeticOpcodes::inc16(state, indexReg) + 4;
  case 0x29: // ADD IX/IY, IX/IY
    return ArithmeticOpcodes::add16(state, indexReg, indexReg) + 4;
  case 0x2B: // DEC IX/IY
    return ArithmeticOpcodes::dec16(state, indexReg) + 4;
  case 0x39: // ADD IX/IY, SP
    return ArithmeticOpcodes::add16(state, indexReg, state.registers.SP) + 4;

  case 0x34: // INC (IX/IY + d)
  {
    byte d = state.getNextByteFromPC();
    state.incPC();
    word address = indexReg + (int8_t)d;

    // INC logic
    byte val = state.memory[address];
    val++;
    state.memory[address] = val;

    // Flags: S, Z, H, P/V, N
    // S: Sign
    if (val & 0x80)
      SET_FLAG(S_FLAG, state.registers);
    else
      CLEAR_FLAG(S_FLAG, state.registers);

    // Z: Zero
    if (val == 0)
      SET_FLAG(Z_FLAG, state.registers);
    else
      CLEAR_FLAG(Z_FLAG, state.registers);

    // H: Half carry
    if ((val & 0x0F) == 0x00)
      SET_FLAG(H_FLAG, state.registers);
    else
      CLEAR_FLAG(H_FLAG, state.registers);

    // P/V: Overflow (V)
    if (val == 0x80)
      SET_FLAG(P_FLAG, state.registers); // Was 0x7F, became 0x80
    else
      CLEAR_FLAG(P_FLAG, state.registers);

    // N: Subtract
    CLEAR_FLAG(N_FLAG, state.registers);

    return 23;
  }
  case 0x35: { // DEC (IX/IY+d)
    signed char offset = (signed char)state.getNextByteFromPC();
    state.incPC();
    word addr = indexReg + offset;
    // Assuming ArithmeticOpcodes::dec8 exists and handles flags
    // This is a placeholder as ArithmeticOpcodes is not in this file
    // For now, keeping the original logic for DEC (IX/IY+d)
    byte val = state.memory[addr];
    val--;
    state.memory[addr] = val;

    // Flags
    if (val & 0x80)
      SET_FLAG(S_FLAG, state.registers);
    else
      CLEAR_FLAG(S_FLAG, state.registers);

    if (val == 0)
      SET_FLAG(Z_FLAG, state.registers);
    else
      CLEAR_FLAG(Z_FLAG, state.registers);

    if ((val & 0x0F) == 0x0F)
      SET_FLAG(H_FLAG, state.registers);
    else
      CLEAR_FLAG(H_FLAG, state.registers);

    if (val == 0x7F)
      SET_FLAG(P_FLAG, state.registers); // Was 0x80, became 0x7F
    else
      CLEAR_FLAG(P_FLAG, state.registers);

    SET_FLAG(N_FLAG, state.registers);

    return 23;
  }
  case 0x36: { // LD (IX/IY+d), n
    signed char offset = (signed char)state.getNextByteFromPC();
    state.incPC();
    byte n = state.getNextByteFromPC();
    state.incPC();
    word addr = indexReg + offset;
    state.memory[addr] = n;
    return 19;
  }
  case 0xCB: // Indexed Bit Operations (DD/FD CB d op)
    return processIndexCB(state, indexReg);

    // LD r, (IX/IY+d)
  case 0x46: // LD B, (IX/IY+d)
    return ld_r_idx(state, indexReg, state.registers.B);
  case 0x4E: // LD C, (IX/IY+d)
    return ld_r_idx(state, indexReg, state.registers.C);
  case 0x56: // LD D, (IX/IY+d)
    return ld_r_idx(state, indexReg, state.registers.D);
  case 0x5E: // LD E, (IX/IY+d)
    return ld_r_idx(state, indexReg, state.registers.E);
  case 0x66: // LD H, (IX/IY+d)
    return ld_r_idx(state, indexReg, state.registers.H);
  case 0x6E: // LD L, (IX/IY+d)
    return ld_r_idx(state, indexReg, state.registers.L);
  case 0x7E: // LD A, (IX/IY+d)
    return ld_r_idx(state, indexReg, state.registers.A);

    // LD (IX/IY+d), r
  case 0x70: // LD (IX/IY+d), B
    return ld_idx_r(state, indexReg, state.registers.B);
  case 0x71: // LD (IX/IY+d), C
    return ld_idx_r(state, indexReg, state.registers.C);
  case 0x72: // LD (IX/IY+d), D
    return ld_idx_r(state, indexReg, state.registers.D);
  case 0x73: // LD (IX/IY+d), E
    return ld_idx_r(state, indexReg, state.registers.E);
  case 0x74: // LD (IX/IY+d), H
    return ld_idx_r(state, indexReg, state.registers.H);
  case 0x75: // LD (IX/IY+d), L
    return ld_idx_r(state, indexReg, state.registers.L);
  case 0x77: // LD (IX/IY+d), A
    return ld_idx_r(state, indexReg, state.registers.A);

  // Indexed Arithmetic / Logic (op (IX+d))
  // ADD A, (IX+d) - 0x86
  case 0x86: {
    byte d = state.getNextByteFromPC();
    state.incPC();
    word addr = indexReg + (int8_t)d;
    return ArithmeticOpcodes::add8(state, state.memory[addr]) +
           15; // 19T total (4 base + 15 add)
  }
  // ADC A, (IX+d) - 0x8E
  case 0x8E: {
    byte d = state.getNextByteFromPC();
    state.incPC();
    word addr = indexReg + (int8_t)d;
    return ArithmeticOpcodes::adc8(state, state.memory[addr]) + 15;
  }
  // SUB (IX+d) - 0x96
  case 0x96: {
    byte d = state.getNextByteFromPC();
    state.incPC();
    word addr = indexReg + (int8_t)d;
    return ArithmeticOpcodes::sub8(state, state.memory[addr]) + 15;
  }
  // SBC A, (IX+d) - 0x9E
  case 0x9E: {
    byte d = state.getNextByteFromPC();
    state.incPC();
    word addr = indexReg + (int8_t)d;
    return ArithmeticOpcodes::sbc8(state, state.memory[addr]) + 15;
  }
  // AND (IX+d) - 0xA6
  case 0xA6: {
    byte d = state.getNextByteFromPC();
    state.incPC();
    word addr = indexReg + (int8_t)d;
    return LogicOpcodes::and8(state, state.memory[addr]) + 15;
  }
  // XOR (IX+d) - 0xAE
  case 0xAE: {
    byte d = state.getNextByteFromPC();
    state.incPC();
    word addr = indexReg + (int8_t)d;
    return LogicOpcodes::xor8(state, state.memory[addr]) + 15;
  }
  // OR (IX+d) - 0xB6
  case 0xB6: {
    byte d = state.getNextByteFromPC();
    state.incPC();
    word addr = indexReg + (int8_t)d;
    return LogicOpcodes::or8(state, state.memory[addr]) + 15;
  }
  // CP (IX+d) - 0xBE
  case 0xBE: {
    byte d = state.getNextByteFromPC();
    state.incPC();
    word addr = indexReg + (int8_t)d;
    return LogicOpcodes::cp8(state, state.memory[addr]) + 15;
  }
  // JP (IX) - 0xE9
  case 0xE9: {
    state.setPC(indexReg);
    return 8;
  }

  default:
    debug("Unknown Index Opcode %02X\n", opcode);
    return 0;
  }
}

int IndexOpcodes::ld_r_idx(ProcessorState &state, emulator_types::word indexReg,
                           emulator_types::byte &reg) {
  byte d = state.getNextByteFromPC();
  state.incPC();
  word address = indexReg + (int8_t)d;
  reg = state.memory[address];
  return 19;
}

int IndexOpcodes::ld_idx_r(ProcessorState &state, emulator_types::word indexReg,
                           emulator_types::byte reg) {
  byte d = state.getNextByteFromPC();
  state.incPC();
  word address = indexReg + (int8_t)d;
  state.memory[address] = reg;
  return 19;
}

int IndexOpcodes::processIndexCB(ProcessorState &state,
                                 emulator_types::word &indexReg) {
  byte d = state.getNextByteFromPC();
  state.incPC();
  byte op = state.getNextByteFromPC();
  state.incPC();

  word address = indexReg + (int8_t)d;

  // Decode CB opcode
  // 0x00-0x3F: Rotates/Shifts
  // 0x40-0x7F: BIT b, (HL) -> b = (op >> 3) & 7
  // 0x80-0xBF: RES b, (HL)
  // 0xC0-0xFF: SET b, (HL)

  if (op >= 0xC0) { // SET b, (HL)
    int bit = (op >> 3) & 7;
    byte val = state.memory[address];
    val |= (1 << bit);
    state.memory[address] = val;
    // Copy to register?
    // Indexed bit ops also copy result to register specified by bottom 3 bits
    // (except instructions where bottom 3 are 110 (0x6)) Standard (HL) ops
    // don't copy. But Index CB usually operates on (IX+d) only.
    return 23;
  } else if (op >= 0x80) { // RES b, (HL)
    int bit = (op >> 3) & 7;
    byte val = state.memory[address];
    val &= ~(1 << bit);
    state.memory[address] = val;
    return 23;
  } else if (op >= 0x40) { // BIT b, (HL)
    int bit = (op >> 3) & 7;
    byte val = state.memory[address];

    bool zero = !((val >> bit) & 1);

    if (zero)
      SET_FLAG(Z_FLAG, state.registers);
    else
      CLEAR_FLAG(Z_FLAG, state.registers);

    SET_FLAG(H_FLAG, state.registers);
    CLEAR_FLAG(N_FLAG, state.registers);
    // S, P/V are typically unknown or set specific way for BIT?
    // BIT behavior for flags: Z set if bit is 0. H set. N reset.
    // P/V unknown? S unknown?
    // Taking standard behavior.
    return 20;
  } else {
    // Rotates
    debug("Unknown Index CB Rotate Opcode %02X\n", op);
    return 0;
  }
}
