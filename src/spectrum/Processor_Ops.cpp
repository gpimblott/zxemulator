#include "Processor.h"
#include "instructions/ArithmeticInstructions.h"
#include "instructions/LogicInstructions.h"

int Processor::exec_loads_8bit(byte opcode) {
  // 0x40 - 0x7F: LD r, r'
  // Binary: 01 ddd sss
  // 000=B, 001=C, 010=D, 011=E, 100=H, 101=L, 110=(HL), 111=A
  // EXCEPTION: 0x76 (01 110 110) is HALT. Caller should handle HALT before
  // calling this, OR we handle it here.
  // We will assume caller handles HALT (0x76) because it's distinct.

  if (opcode == 0x76) {
    return 0; // Should not happen if caller filters, or treat as NOP here?
              // The main loop handles HALT state specifically.
  }

  int destIndex = (opcode >> 3) & 7;
  int srcIndex = opcode & 7;

  byte value = 0;
  int cycles = 4;

  // READ SOURCE
  switch (srcIndex) {
  case 0:
    value = state.registers.B;
    break;
  case 1:
    value = state.registers.C;
    break;
  case 2:
    value = state.registers.D;
    break;
  case 3:
    value = state.registers.E;
    break;
  case 4:
    value = state.registers.H;
    break;
  case 5:
    value = state.registers.L;
    break;
  case 6: // (HL)
    value = state.memory[state.registers.HL];
    cycles = 7;
    break;
  case 7:
    value = state.registers.A;
    break;
  }

  // WRITE DEST
  if (destIndex == 6) { // LD (HL), r
    state.memory.fastWrite(state.registers.HL, value);
    cycles = 7; // LD (HL), r is 7 cycles
  } else {
    // LD r, (HL) is 7 cycles, LD r, r is 4.
    // usage of 'cycles' variable handles the src=(HL) case => 7.
    // If dest=(HL), we set cycles=7 above.
    // If BOTH are (HL)? 0x76 HALT. Handled.

    switch (destIndex) {
    case 0:
      state.registers.B = value;
      break;
    case 1:
      state.registers.C = value;
      break;
    case 2:
      state.registers.D = value;
      break;
    case 3:
      state.registers.E = value;
      break;
    case 4:
      state.registers.H = value;
      break;
    case 5:
      state.registers.L = value;
      break;
    case 7:
      state.registers.A = value;
      break;
    }
  }

  return cycles;
}

int Processor::exec_alu_8bit(byte opcode) {
  // 0x80 - 0xBF: ALU A, r
  // Binary: 10 ooo sss
  // Ops: 0=ADD, 1=ADC, 2=SUB, 3=SBC, 4=AND, 5=XOR, 6=OR, 7=CP

  int opIndex = (opcode >> 3) & 7;
  int srcIndex = opcode & 7;

  byte value = 0;
  int cycles = 4;

  // READ SOURCE
  switch (srcIndex) {
  case 0:
    value = state.registers.B;
    break;
  case 1:
    value = state.registers.C;
    break;
  case 2:
    value = state.registers.D;
    break;
  case 3:
    value = state.registers.E;
    break;
  case 4:
    value = state.registers.H;
    break;
  case 5:
    value = state.registers.L;
    break;
  case 6: // (HL)
    value = state.memory[state.registers.HL];
    cycles = 7;
    break;
  case 7:
    value = state.registers.A;
    break;
  }

  // EXECUTE OP
  switch (opIndex) {
  case 0:
    Arithmetic::add8(state, value);
    break;
  case 1:
    Arithmetic::adc8(state, value);
    break;
  case 2:
    Arithmetic::sub8(state, value);
    break;
  case 3:
    Arithmetic::sbc8(state, value);
    break;
  case 4:
    Logic::and8(state, value);
    break;
  case 5:
    Logic::xor8(state, value);
    break;
  case 6:
    Logic::or8(state, value);
    break;
  case 7:
    Arithmetic::cp8(state, value);
    break;
  }

  return cycles;
}
