#include "Processor.h"
#include "instructions/ArithmeticInstructions.h"
#include "instructions/BitInstructions.h"
#include "instructions/ControlInstructions.h"
#include "instructions/IOInstructions.h"
#include "instructions/LoadInstructions.h"
#include "instructions/LogicInstructions.h"

// ============================================================================
// Extended (ED) Opcode Handler
// ============================================================================
int Processor::exec_ed_opcode() {
  byte extOpcode = state.getNextByteFromPC();
  state.registers.PC++; // Increment past the extended opcode

  int cycles = 0;

  switch (extOpcode) {
  // LDS
  case 0x47:
    state.registers.I = state.registers.A;
    cycles = 9;
    break;
  case 0x57:
    state.registers.A = state.registers.I;
    cycles = 9;
    {
      byte f = state.registers.F & C_FLAG;
      if (state.registers.A == 0)
        f |= Z_FLAG;
      if (state.registers.A & 0x80)
        f |= S_FLAG;
      if (state.registers.IFF2)
        f |= P_FLAG;
      state.registers.F = f;
    }
    break;

  // LD A, R / LD R, A
  case 0x4F:
    state.registers.R = state.registers.A;
    cycles = 9;
    break;
  case 0x5F:
    // Preserves bit 7 of R? R is 7 bits refresh.
    // Actually Z80 R register is 8 bits (top bit changes roughly).
    // Emulator often keeps R as 8 bits.
    state.registers.A = state.registers.R;
    cycles = 9;
    {
      byte f = state.registers.F & C_FLAG;
      if (state.registers.A == 0)
        f |= Z_FLAG;
      if (state.registers.A & 0x80)
        f |= S_FLAG;
      if (state.registers.IFF2)
        f |= P_FLAG;
      CLEAR_FLAG(N_FLAG, state.registers);
      CLEAR_FLAG(H_FLAG, state.registers);
      // Merge with existing flags (C preserved)
      // wait, I constructed 'f' based on C_FLAG + new flags.
      // existing code did: byte f = state.registers.F & C_FLAG; ...
      // state.registers.F = f; This CLEARED other flags (like H/N). LD A, I
      // documentation: S, Z, H=0, P/V=IFF2, N=0, C preserved. So code is
      // correct.
      state.registers.F = f;
    }
    break;

  // RRD / RLD
  case 0x67:
    Bit::rrd(state);
    cycles = 18;
    break;
  case 0x6F:
    Bit::rld(state);
    cycles = 18;
    break;

  // RETI
  case 0x4D:
    cycles = Control::reti(state);
    break;

  // RETN
  case 0x45:
    cycles = Control::retn(state);
    break;
  // IN r, (C)
  case 0x40:
    IO::in_r_c(state, state.registers.B);
    cycles = 12;
    break;
  case 0x48:
    IO::in_r_c(state, state.registers.C);
    cycles = 12;
    break;
  case 0x50:
    IO::in_r_c(state, state.registers.D);
    cycles = 12;
    break;
  case 0x58:
    IO::in_r_c(state, state.registers.E);
    cycles = 12;
    break;
  case 0x60:
    IO::in_r_c(state, state.registers.H);
    cycles = 12;
    break;
  case 0x68:
    IO::in_r_c(state, state.registers.L);
    cycles = 12;
    break;
  case 0x78:
    IO::in_r_c(state, state.registers.A);
    cycles = 12;
    break;

  // SBC HL, rr
  case 0x42:
    Arithmetic::sbc16(state, state.registers.HL, state.registers.BC);
    cycles = 15;
    break;
  case 0x52:
    Arithmetic::sbc16(state, state.registers.HL, state.registers.DE);
    cycles = 15;
    break;
  case 0x62:
    Arithmetic::sbc16(state, state.registers.HL, state.registers.HL);
    cycles = 15;
    break;
  case 0x72:
    Arithmetic::sbc16(state, state.registers.HL, state.registers.SP);
    cycles = 15;
    break;

  // ADC HL, rr
  case 0x4A:
    Arithmetic::adc16(state, state.registers.HL, state.registers.BC);
    cycles = 15;
    break;
  case 0x5A:
    Arithmetic::adc16(state, state.registers.HL, state.registers.DE);
    cycles = 15;
    break;
  case 0x6A:
    Arithmetic::adc16(state, state.registers.HL, state.registers.HL);
    cycles = 15;
    break;
  case 0x7A:
    Arithmetic::adc16(state, state.registers.HL, state.registers.SP);
    cycles = 15;
    break;

  // LD (nn), rr - Consumes 2 byte operand (nn)
  case 0x43:
    Load::ld_nn_rr(state, state.getNextWordFromPC(), state.registers.BC);
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x53:
    Load::ld_nn_rr(state, state.getNextWordFromPC(), state.registers.DE);
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x63:
    Load::ld_nn_rr(state, state.getNextWordFromPC(), state.registers.HL);
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x73:
    Load::ld_nn_rr(state, state.getNextWordFromPC(), state.registers.SP);
    state.registers.PC += 2;
    cycles = 20;
    break;

  // LD rr, (nn) - Consumes 2 byte operand (nn)
  case 0x4B:
    Load::ld_rr_nn(state, state.registers.BC, state.getNextWordFromPC());
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x5B:
    Load::ld_rr_nn(state, state.registers.DE, state.getNextWordFromPC());
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x6B:
    Load::ld_rr_nn(state, state.registers.HL, state.getNextWordFromPC());
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x7B:
    Load::ld_rr_nn(state, state.registers.SP, state.getNextWordFromPC());
    state.registers.PC += 2;
    cycles = 20;
    break;

  case 0x44: // NEG
    Arithmetic::neg8(state);
    cycles = 8;
    break;

  // IM
  case 0x46:
    state.setInterruptMode(0);
    cycles = 8;
    break;
  case 0x56:
    state.setInterruptMode(1);
    cycles = 8;
    break;
  case 0x5E:
    state.setInterruptMode(2);
    cycles = 8;
    break;

  // Block Ops
  case 0xA0:
    cycles = Load::ldi(state);
    break;
  case 0xA8:
    cycles = Load::ldd(state);
    break;
  case 0xB0:
    cycles = Load::ldir(state);
    break;
  case 0xB8:
    cycles = Load::lddr(state);
    break;
  case 0xA1:
    cycles = Control::cpi(state);
    break;
  case 0xA9:
    cycles = Control::cpd(state);
    break;
  case 0xB1:
    cycles = Control::cpir(state);
    break;
  case 0xB9:
    cycles = Control::cpdr(state);
    break;

  // Block I/O
  case 0xA2:
    cycles = cycles = IO::ini(state);
    break;
  case 0xB2:
    cycles = cycles = IO::inir(state);
    break;
  case 0xAA:
    cycles = cycles = IO::ind(state);
    break;
  case 0xBA:
    cycles = cycles = IO::indr(state);
    break;
  case 0xA3:
    cycles = cycles = IO::outi(state);
    break;
  case 0xB3:
    cycles = cycles = IO::otir(state);
    break;
  case 0xAB:
    cycles = cycles = IO::outd(state);
    break;
  case 0xBB:
    cycles = cycles = IO::otdr(state);
    break;

  default:
    cycles = 8; // NOP (approx) for unknown ED to prevent infinite loops
    break;
  }
  return cycles;
}

// ============================================================================
// Bit (CB) Opcode Handler
// ============================================================================
int Processor::exec_cb_opcode() {
  byte cbOpcode = state.getNextByteFromPC();
  state.registers.PC++;

  int x = (cbOpcode >> 6) & 3;
  int y = (cbOpcode >> 3) & 7;
  int z = cbOpcode & 7;

  byte *regPtr = nullptr;
  byte memVal = 0;
  bool isMem = false;
  word hlAddr = state.registers.HL;

  // Decode register
  switch (z) {
  case 0:
    regPtr = &state.registers.B;
    break;
  case 1:
    regPtr = &state.registers.C;
    break;
  case 2:
    regPtr = &state.registers.D;
    break;
  case 3:
    regPtr = &state.registers.E;
    break;
  case 4:
    regPtr = &state.registers.H;
    break;
  case 5:
    regPtr = &state.registers.L;
    break;
  case 6: // (HL)
    isMem = true;
    memVal = state.memory[hlAddr];
    regPtr = &memVal;
    break;
  case 7:
    regPtr = &state.registers.A;
    break;
  }

  int cycles = 8;
  if (isMem)
    cycles = (x == 1) ? 12 : 15; // BIT (HL)=12, others=15
  else
    cycles = 8;

  if (x == 0) { // Rotate/Shift
    switch (y) {
    case 0:
      Bit::rlc(state, *regPtr);
      break;
    case 1:
      Bit::rrc(state, *regPtr);
      break;
    case 2:
      Bit::rl(state, *regPtr);
      break;
    case 3:
      Bit::rr(state, *regPtr);
      break;
    case 4:
      Bit::sla(state, *regPtr);
      break;
    case 5:
      Bit::sra(state, *regPtr);
      break;
    case 6:
      Bit::sll(state, *regPtr);
      break; // SLL (undocumented)
    case 7:
      Bit::srl(state, *regPtr);
      break;
    }
  } else if (x == 1) { // BIT
    if (isMem) {
      // Z80 Undocumented: BIT n, (HL) sets X/Y from internal register (MEMPTR
      // high byte), which is H (High byte of HL) Here, MEMPTR = HL. So pass
      // (hlAddr >> 8) as the source for X/Y flags, but 'val' for Z flag test.
      // We need to modify Bit::bit signature or handle it here.
      // Let's modify Bit::bit to take an optional 'undocSource'
      // OR: handle flags manually here.
      Bit::bitMem(state, y, *regPtr, (hlAddr >> 8));
      return cycles;
    } else {
      Bit::bit(state, y, *regPtr);
    }
  } else if (x == 2) { // RES
    Bit::res(state, y, *regPtr);
  } else if (x == 3) { // SET
    Bit::set(state, y, *regPtr);
  }

  // Write back if memory and NOT BIT (BIT doesn't modify)
  if (isMem && x != 1) {
    writeMem(hlAddr, *regPtr);
  }

  return cycles;
}
