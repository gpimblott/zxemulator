#include "Processor.h"
#include "instructions/ArithmeticInstructions.h"
#include "instructions/BitInstructions.h"
#include "instructions/ControlInstructions.h"
#include "instructions/IOInstructions.h"
#include "instructions/LoadInstructions.h"
#include "instructions/LogicInstructions.h"

// ============================================================================
// Index (IX/IY) Instructions
// ============================================================================
int Processor::exec_index_opcode(byte prefix) {
  // Determine Index Register
  word &idx = (prefix == 0xDD) ? state.registers.IX : state.registers.IY;

  // Fetch opcode
  byte opcode = state.getNextByteFromPC();
  state.registers.PC++;

  // Handle DD CB d <opcode>
  if (opcode == 0xCB) {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    byte cbOp = state.getNextByteFromPC();
    state.registers.PC++;

    word addr = idx + (int8_t)d;
    byte val = state.memory[addr];

    int x = (cbOp >> 6) & 3;
    int y = (cbOp >> 3) & 7;
    int z = cbOp & 7;

    // Most CB ops operate on 'val' then write back
    // BIT (x=1) does not write back.
    // Logic is similar to standard CB but on (IX+d).
    // Note: Z80 undocumented behavior: result is ALSO copied to register
    // specified by 'z' (if not 6).
    // We MUST implement this for game compatibility (e.g. Jetpac).

    int cycles = 23; // Usually 23 for indexed bit ops

    // Lambda for undocumented writeback
    auto setUndocReg = [&](int r, byte v) {
      switch (r) {
      case 0:
        state.registers.B = v;
        break;
      case 1:
        state.registers.C = v;
        break;
      case 2:
        state.registers.D = v;
        break;
      case 3:
        state.registers.E = v;
        break;
      case 4:
        state.registers.H = v;
        break;
      case 5:
        state.registers.L = v;
        break;
      case 7:
        state.registers.A = v;
        break;
      }
    };

    if (x == 0) { // Rotate/Shift
      switch (y) {
      case 0:
        Bit::rlc(state, val);
        break;
      case 1:
        Bit::rrc(state, val);
        break;
      case 2:
        Bit::rl(state, val);
        break;
      case 3:
        Bit::rr(state, val);
        break;
      case 4:
        Bit::sla(state, val);
        break;
      case 5:
        Bit::sra(state, val);
        break;
      case 6:
        Bit::sll(state, val);
        break;
      case 7:
        Bit::srl(state, val);
        break;
      }
      writeMem(addr, val);
      // Undocumented register writeback
      if (z != 6)
        setUndocReg(z, val);
    } else if (x == 1) { // BIT
      // Z80 Undocumented: BIT n, (IX+d).
      // X/Y flags taken from High Byte of (IX+d) address.
      // 'addr' is the calculated effective address.
      Bit::bitMem(state, y, val, (addr >> 8));
      cycles = 20;       // BIT is 20
                         // No writeback to memory OR register
    } else if (x == 2) { // RES
      Bit::res(state, y, val);
      writeMem(addr, val);
      // Undocumented register writeback
      if (z != 6)
        setUndocReg(z, val);
    } else if (x == 3) { // SET
      Bit::set(state, y, val);
      writeMem(addr, val);
      // Undocumented register writeback
      if (z != 6)
        setUndocReg(z, val);
    }

    return cycles;
  }

  // Standard Index Opcodes
  // Helper for High/Low byte access (Little Endian host assumed)
  byte *idxL = (byte *)&idx;
  byte *idxH = (byte *)&idx + 1;

  int cycles = 0;

  switch (opcode) {
  // 09, 19, 29, 39: ADD IX, rr
  case 0x09:
    Arithmetic::add16(state, idx, state.registers.BC);
    cycles = 15;
    break;
  case 0x19:
    Arithmetic::add16(state, idx, state.registers.DE);
    cycles = 15;
    break;
  case 0x29:
    Arithmetic::add16(state, idx, idx);
    cycles = 15;
    break;
  case 0x39:
    Arithmetic::add16(state, idx, state.registers.SP);
    cycles = 15;
    break;

  // 21 nn: LD IX, nn
  case 0x21: {
    idx = state.getNextWordFromPC();
    state.registers.PC += 2;
    cycles = 14;
    break;
  }

  // 22 nn: LD (nn), IX
  case 0x22: {
    word addr = state.getNextWordFromPC();
    state.registers.PC += 2;
    writeMem(addr, *idxL);     // Low byte
    writeMem(addr + 1, *idxH); // High byte
    cycles = 20;
    break;
  }

  // 2A nn: LD IX, (nn)
  case 0x2A: {
    word addr = state.getNextWordFromPC();
    state.registers.PC += 2;
    *idxL = state.memory[addr];
    *idxH = state.memory[addr + 1];
    cycles = 20;
    break;
  }

  // 23: INC IX
  case 0x23:
    idx++;
    cycles = 10;
    break;

  // 2B: DEC IX
  case 0x2B:
    idx--;
    cycles = 10;
    break;

  // 24: INC IXH
  case 0x24: {
    Arithmetic::inc8(state, *idxH);
    cycles = 8;
    break;
  }
  // 25: DEC IXH
  case 0x25: {
    Arithmetic::dec8(state, *idxH);
    cycles = 8;
    break;
  }
  // 2C: INC IXL
  case 0x2C: {
    Arithmetic::inc8(state, *idxL);
    cycles = 8;
    break;
  }
  // 2D: DEC IXL
  case 0x2D: {
    Arithmetic::dec8(state, *idxL);
    cycles = 8;
    break;
  }
  // 26: LD IXH, n
  case 0x26: {
    byte n = state.getNextByteFromPC();
    state.registers.PC++;
    *idxH = n;
    cycles = 11;
    break;
  }
  // 2E: LD IXL, n
  case 0x2E: {
    byte n = state.getNextByteFromPC();
    state.registers.PC++;
    *idxL = n;
    cycles = 11;
    break;
  }

  // 34: INC (IX+d)
  case 0x34: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    word addr = idx + (int8_t)d;
    byte val = state.memory[addr];
    // Use helper for correct flags (H, P/V)
    Arithmetic::inc8(state, val);
    writeMem(addr, val);
    cycles = 23;
    break;
  }

  // 35: DEC (IX+d)
  case 0x35: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    word addr = idx + (int8_t)d;
    byte val = state.memory[addr];
    // Use helper for correct flags (H, P/V)
    Arithmetic::dec8(state, val);
    writeMem(addr, val);
    cycles = 23;
    break;
  }

  // 36: LD (IX+d), n
  case 0x36: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    byte n = state.getNextByteFromPC();
    state.registers.PC++;
    writeMem(idx + (int8_t)d, n);
    cycles = 19;
    break;
  }

  // UNDOCUMENTED / REQUESTED OPCODES
  // 44: LD B, IXH/IYH
  case 0x44:
    state.registers.B = *idxH;
    cycles = 8;
    break;
  // 45: LD B, IXL/IYL -- standard mapping LD B,L -> LD B, IXL
  case 0x45:
    state.registers.B = *idxL;
    cycles = 8;
    break;

  // 4C: LD C, IXH
  case 0x4C:
    state.registers.C = *idxH;
    cycles = 8;
    break;
  // 4D: LD C, IXL (Requested)
  case 0x4D:
    state.registers.C = *idxL;
    cycles = 8;
    break;

  // 54: LD D, IXH
  case 0x54:
    state.registers.D = *idxH;
    cycles = 8;
    break;
  // 55: LD D, IXL
  case 0x55:
    state.registers.D = *idxL;
    cycles = 8;
    break;

  // 5C: LD E, IXH
  case 0x5C:
    state.registers.E = *idxH;
    cycles = 8;
    break;
  // 5D: LD E, IXL
  case 0x5D:
    state.registers.E = *idxL;
    cycles = 8;
    break;

  // 60: LD IXH, B
  case 0x60:
    *idxH = state.registers.B;
    cycles = 8;
    break;
  // 61: LD IXH, C
  case 0x61:
    *idxH = state.registers.C;
    cycles = 8;
    break;
  // 62: LD IXH, D
  case 0x62:
    *idxH = state.registers.D;
    cycles = 8;
    break;
  // 63: LD IXH, E
  case 0x63:
    *idxH = state.registers.E;
    cycles = 8;
    break;
  // 64: LD IXH, IXH (NOP)
  case 0x64:
    cycles = 8;
    break;
  // 65: LD IXH, IXL
  case 0x65:
    *idxH = *idxL;
    cycles = 8;
    break;

  // 67: LD IXH, A (Undocumented)
  case 0x67:
    *idxH = state.registers.A;
    cycles = 8;
    break;

  // 68: LD IXL, B
  case 0x68:
    *idxL = state.registers.B;
    cycles = 8;
    break;
  // 69: LD IXL, C
  case 0x69:
    *idxL = state.registers.C;
    cycles = 8;
    break;
  // 6A: LD IXL, D (Undocumented)
  case 0x6A:
    *idxL = state.registers.D;
    cycles = 8;
    break;
  // 6B: LD IXL, E (Undocumented)
  case 0x6B:
    *idxL = state.registers.E;
    cycles = 8;
    break;

  // 6C: LD IXL, H -> LD IXL, IXH
  case 0x6C:
    *idxL = *idxH;
    cycles = 8;
    break;
  // 6D: LD IXL, L -> LD IXL, IXL
  case 0x6D:
    cycles = 8;
    break;

  // 6F: LD IXL, A (Undocumented)
  case 0x6F:
    *idxL = state.registers.A;
    cycles = 8;
    break;

  // 70-75, 77: LD (IX+d), r
  case 0x70: { // LD (IX+d), B
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    writeMem(idx + (int8_t)d, state.registers.B);
    cycles = 19;
    break;
  }
  case 0x71: { // LD (IX+d), C
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    writeMem(idx + (int8_t)d, state.registers.C);
    cycles = 19;
    break;
  }
  case 0x72: { // LD (IX+d), D
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    writeMem(idx + (int8_t)d, state.registers.D);
    cycles = 19;
    break;
  }
  case 0x73: { // LD (IX+d), E
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    writeMem(idx + (int8_t)d, state.registers.E);
    cycles = 19;
    break;
  }
  case 0x74: { // LD (IX+d), H
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    writeMem(idx + (int8_t)d, state.registers.H);
    cycles = 19;
    break;
  }
  case 0x75: { // LD (IX+d), L
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    writeMem(idx + (int8_t)d, state.registers.L);
    cycles = 19;
    break;
  }
  case 0x77: { // LD (IX+d), A
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    writeMem(idx + (int8_t)d, state.registers.A);
    cycles = 19;
    break;
  }

  // 7C: LD A, IXH
  case 0x7C:
    state.registers.A = *idxH;
    cycles = 8;
    break;
  // 7D: LD A, IXL
  case 0x7D:
    state.registers.A = *idxL;
    cycles = 8;
    break;

  // 7E: LD A, (IX+d)
  case 0x7E: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    state.registers.A = state.memory[idx + (int8_t)d];
    cycles = 19;
    break;
  }

  // Undocumented: Arithmetic with IXH/IXL
  case 0x84: // ADD A, IXH
    Arithmetic::add8(state, *idxH);
    cycles = 8;
    break;
  case 0x85: // ADD A, IXL
    Arithmetic::add8(state, *idxL);
    cycles = 8;
    break;
  case 0x8C: // ADC A, IXH
    Arithmetic::adc8(state, *idxH);
    cycles = 8;
    break;
  case 0x8D: // ADC A, IXL
    Arithmetic::adc8(state, *idxL);
    cycles = 8;
    break;
  case 0x94: // SUB IXH
    Arithmetic::sub8(state, *idxH);
    cycles = 8;
    break;
  case 0x95: // SUB IXL
    Arithmetic::sub8(state, *idxL);
    cycles = 8;
    break;
  case 0x9C: // SBC A, IXH
    Arithmetic::sbc8(state, *idxH);
    cycles = 8;
    break;
  case 0x9D: // SBC A, IXL
    Arithmetic::sbc8(state, *idxL);
    cycles = 8;
    break;
  case 0xA4: // AND IXH
    Logic::and8(state, *idxH);
    cycles = 8;
    break;
  case 0xA5: // AND IXL
    Logic::and8(state, *idxL);
    cycles = 8;
    break;
  case 0xAC: // XOR IXH
    Logic::xor8(state, *idxH);
    cycles = 8;
    break;
  case 0xAD: // XOR IXL
    Logic::xor8(state, *idxL);
    cycles = 8;
    break;
  // B4, B5: OR - already implemented above
  case 0xBC: // CP IXH
    Arithmetic::cp8(state, *idxH);
    cycles = 8;
    break;
  case 0xBD: // CP IXL
    Arithmetic::cp8(state, *idxL);
    cycles = 8;
    break;

  // ALU (IX+d)
  case 0x86: { // ADD A, (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    Arithmetic::add8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0x8E: { // ADC A, (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    Arithmetic::adc8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0x96: { // SUB (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    Arithmetic::sub8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0x9E: { // SBC A, (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    Arithmetic::sbc8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0xA6: { // AND (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    Logic::and8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0xAE: { // XOR (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    Logic::xor8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0xB6: { // OR (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    Logic::or8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0xBE: { // CP (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    Arithmetic::cp8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  // 46: LD B, (IX+d)
  case 0x46: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    state.registers.B = state.memory[idx + (int8_t)d];
    cycles = 19;
    break;
  }
  // 4E, 56, 5E, 66, 6E
  case 0x4E: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    state.registers.C = state.memory[idx + (int8_t)d];
    cycles = 19;
    break;
  }
  case 0x56: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    state.registers.D = state.memory[idx + (int8_t)d];
    cycles = 19;
    break;
  }
  case 0x5E: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    state.registers.E = state.memory[idx + (int8_t)d];
    cycles = 19;
    break;
  }
  case 0x66: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    state.registers.H = state.memory[idx + (int8_t)d];
    cycles = 19;
    break;
  }
  case 0x6E: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    state.registers.L = state.memory[idx + (int8_t)d];
    cycles = 19;
    break;
  }

  // B4: OR IXH (Requested)
  case 0xB4: {
    Logic::or8(state, *idxH);
    cycles = 8;
    break;
  }
  // B5: OR IXL (Requested)
  case 0xB5: {
    Logic::or8(state, *idxL);
    cycles = 8;
    break;
  }

  case 0xE1: {
    idx = Load::pop16(state);
    cycles = 14;
    break;
  }

  // 99: SBC A, C (Prefix ignored)
  case 0x99: {
    Arithmetic::sbc8(state, state.registers.C);
    cycles = 4;
    break;
  }

  // E5: PUSH IX
  case 0xE5: {
    Load::push16(state, idx);
    cycles = 15;
    break;
  }

  // E3: EX (SP), IX
  case 0xE3: {
    byte low = state.memory[state.registers.SP];
    byte high = state.memory[state.registers.SP + 1];
    writeMem(state.registers.SP, idx & 0xFF);
    writeMem(state.registers.SP + 1, (idx >> 8) & 0xFF);
    idx = (high << 8) | low;
    cycles = 23;
    break;
  }

  // E9: JP (IX)
  case 0xE9: {
    state.registers.PC = idx;
    cycles = 8;
    break;
  }

  // F9: LD SP, IX
  case 0xF9: {
    state.registers.SP = idx;
    cycles = 10;
    break;
  }

  // I/O Instructions - IX/IY prefix is ignored
  case 0xD3: { // OUT (n), A
    byte port = state.getNextByteFromPC();
    state.registers.PC++;
    cycles = IO::out_n_a(state, port);
    break;
  }
  case 0xDB: { // IN A, (n)
    byte port = state.getNextByteFromPC();
    state.registers.PC++;
    cycles = IO::in_a_n(state, port);
    break;
  }

  default:

    // Handle unhandled index ops as standard opcodes with no displacement?
    // Or if standard op uses HL, use IX?
    // Z80 Rule: If opcode not index-aware, execute as standard.
    // If it accessed (HL), it accesses (IX+d).
    // BUT my switch handles (IX+d) cases explicitly (7E, 46, etc).
    // What if it's ADD A, (IX+d) (86)?
    // I need to implement ALU ops!
    // 86: ADD A, (IX+d)
    // 96: SUB (IX+d)
    // A6: AND (IX+d)
    // B6: OR (IX+d)
    // BE: CP (IX+d)
    // These are ESSENTIAL.
    state.registers
        .PC--; // Rewind? No, standard dispatch in executeFrame is separated.
    // Debugging loop:
    printf("Unknown Index Opcode %02X %02X\n", prefix, opcode);
    cycles = 4;
    break;
  }

  return cycles;
}
