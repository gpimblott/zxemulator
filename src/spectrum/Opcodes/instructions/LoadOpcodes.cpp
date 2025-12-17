// File: (LoadOpcodes.cpp)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "LoadOpcodes.h"
#include "../../../utils/debug.h"

/**
 * Add the opcodes to the catalogue
 * @param state
 * @param catalogue
 */
LoadOpcodes::LoadOpcodes() : OpCodeProvider() {
  createOpCode(LD_DE_XX, "LD_DE_XX", processLD_DE_XX);
  createOpCode(LD_HL_N, "LD_HL_N", processLD_HL_N);
  // createOpCode(LD_B_A, "LD_B_A", processLD_B_A); // Replaced by lambda
  // createOpCode(LD_H_D, "LD_H_D", processLD_H_D); // Replaced by lambda
  // createOpCode(LD_L_E, "LD_L_E", processLD_L_E); // Replaced by lambda
  // createOpCode(LD_A_X, "LD_A_X", processLD_A_X); // Replaced by lambda (0x3E
  // is LD A, n)

  createOpCode(LD_BC_NN, "LD_BC_NN", processLD_BC_NN);
  createOpCode(LD_DE_NN, "LD_DE_NN", processLD_DE_NN);
  createOpCode(LD_HL_NN_IMM, "LD_HL_NN_IMM", processLD_HL_NN_IMM);
  createOpCode(LD_SP_NN, "LD_SP_NN", processLD_SP_NN);

  createOpCode(LD_NN_HL, "LD_NN_HL", processLD_NN_HL);
  createOpCode(LD_HL_NN, "LD_HL_NN", processLD_HL_NN);
  createOpCode(LD_NN_A, "LD_NN_A", processLD_NN_A);
  createOpCode(LD_A_NN, "LD_A_NN", processLD_A_NN);
  createOpCode(LD_SP_HL, "LD_SP_HL", processLD_SP_HL);

  // LD (HL), r
  createOpCode(0x70, "LD (HL), B", [](ProcessorState &s) {
    s.memory[s.registers.HL] = s.registers.B;
    return 7;
  });
  createOpCode(0x71, "LD (HL), C", [](ProcessorState &s) {
    s.memory[s.registers.HL] = s.registers.C;
    return 7;
  });
  createOpCode(0x72, "LD (HL), D", [](ProcessorState &s) {
    s.memory[s.registers.HL] = s.registers.D;
    return 7;
  });
  createOpCode(0x73, "LD (HL), E", [](ProcessorState &s) {
    s.memory[s.registers.HL] = s.registers.E;
    return 7;
  });
  createOpCode(0x74, "LD (HL), H", [](ProcessorState &s) {
    s.memory[s.registers.HL] = s.registers.H;
    return 7;
  });
  createOpCode(0x75, "LD (HL), L", [](ProcessorState &s) {
    s.memory[s.registers.HL] = s.registers.L;
    return 7;
  });
  // 76 is HALT
  createOpCode(0x77, "LD (HL), A", [](ProcessorState &s) {
    s.memory[s.registers.HL] = s.registers.A;
    return 7;
  });

  // LD r, (HL)
  createOpCode(0x46, "LD B, (HL)", [](ProcessorState &s) {
    s.registers.B = s.memory[s.registers.HL];
    return 7;
  });
  createOpCode(0x4E, "LD C, (HL)", [](ProcessorState &s) {
    s.registers.C = s.memory[s.registers.HL];
    return 7;
  });
  createOpCode(0x56, "LD D, (HL)", [](ProcessorState &s) {
    s.registers.D = s.memory[s.registers.HL];
    return 7;
  });
  createOpCode(0x5E, "LD E, (HL)", [](ProcessorState &s) {
    s.registers.E = s.memory[s.registers.HL];
    return 7;
  });
  createOpCode(0x66, "LD H, (HL)", [](ProcessorState &s) {
    s.registers.H = s.memory[s.registers.HL];
    return 7;
  });
  createOpCode(0x6E, "LD L, (HL)", [](ProcessorState &s) {
    s.registers.L = s.memory[s.registers.HL];
    return 7;
  });
  createOpCode(0x7E, "LD A, (HL)", [](ProcessorState &s) {
    s.registers.A = s.memory[s.registers.HL];
    return 7;
  });

  // LD r, r' (0x40 - 0x7F excluding HALT and (HL))
  // LD B, r
  createOpCode(0x40, "LD B, B", [](ProcessorState &s) {
    return 4;
  }); // NOP effectively but correct timing
  createOpCode(0x41, "LD B, C", [](ProcessorState &s) {
    s.registers.B = s.registers.C;
    return 4;
  });
  createOpCode(0x42, "LD B, D", [](ProcessorState &s) {
    s.registers.B = s.registers.D;
    return 4;
  });
  createOpCode(0x43, "LD B, E", [](ProcessorState &s) {
    s.registers.B = s.registers.E;
    return 4;
  });
  createOpCode(0x44, "LD B, H", [](ProcessorState &s) {
    s.registers.B = s.registers.H;
    return 4;
  });
  createOpCode(0x45, "LD B, L", [](ProcessorState &s) {
    s.registers.B = s.registers.L;
    return 4;
  });
  createOpCode(0x47, "LD B, A", [](ProcessorState &s) {
    s.registers.B = s.registers.A;
    return 4;
  });

  // LD C, r
  createOpCode(0x48, "LD C, B", [](ProcessorState &s) {
    s.registers.C = s.registers.B;
    return 4;
  });
  createOpCode(0x49, "LD C, C", [](ProcessorState &s) { return 4; });
  createOpCode(0x4A, "LD C, D", [](ProcessorState &s) {
    s.registers.C = s.registers.D;
    return 4;
  });
  createOpCode(0x4B, "LD C, E", [](ProcessorState &s) {
    s.registers.C = s.registers.E;
    return 4;
  });
  createOpCode(0x4C, "LD C, H", [](ProcessorState &s) {
    s.registers.C = s.registers.H;
    return 4;
  });
  createOpCode(0x4D, "LD C, L", [](ProcessorState &s) {
    s.registers.C = s.registers.L;
    return 4;
  });
  createOpCode(0x4F, "LD C, A", [](ProcessorState &s) {
    s.registers.C = s.registers.A;
    return 4;
  });

  // LD D, r
  createOpCode(0x50, "LD D, B", [](ProcessorState &s) {
    s.registers.D = s.registers.B;
    return 4;
  });
  createOpCode(0x51, "LD D, C", [](ProcessorState &s) {
    s.registers.D = s.registers.C;
    return 4;
  });
  createOpCode(0x52, "LD D, D", [](ProcessorState &s) { return 4; });
  createOpCode(0x53, "LD D, E", [](ProcessorState &s) {
    s.registers.D = s.registers.E;
    return 4;
  });
  createOpCode(0x54, "LD D, H", [](ProcessorState &s) {
    s.registers.D = s.registers.H;
    return 4;
  });
  createOpCode(0x55, "LD D, L", [](ProcessorState &s) {
    s.registers.D = s.registers.L;
    return 4;
  });
  createOpCode(0x57, "LD D, A", [](ProcessorState &s) {
    s.registers.D = s.registers.A;
    return 4;
  });

  // LD E, r
  createOpCode(0x58, "LD E, B", [](ProcessorState &s) {
    s.registers.E = s.registers.B;
    return 4;
  });
  createOpCode(0x59, "LD E, C", [](ProcessorState &s) {
    s.registers.E = s.registers.C;
    return 4;
  });
  createOpCode(0x5A, "LD E, D", [](ProcessorState &s) {
    s.registers.E = s.registers.D;
    return 4;
  });
  createOpCode(0x5B, "LD E, E", [](ProcessorState &s) { return 4; });
  createOpCode(0x5C, "LD E, H", [](ProcessorState &s) {
    s.registers.E = s.registers.H;
    return 4;
  });
  createOpCode(0x5D, "LD E, L", [](ProcessorState &s) {
    s.registers.E = s.registers.L;
    return 4;
  });
  createOpCode(0x5F, "LD E, A", [](ProcessorState &s) {
    s.registers.E = s.registers.A;
    return 4;
  });

  // LD H, r
  createOpCode(0x60, "LD H, B", [](ProcessorState &s) {
    s.registers.H = s.registers.B;
    return 4;
  });
  createOpCode(0x61, "LD H, C", [](ProcessorState &s) {
    s.registers.H = s.registers.C;
    return 4;
  });
  createOpCode(0x62, "LD H, D", [](ProcessorState &s) {
    s.registers.H = s.registers.D;
    return 4;
  });
  createOpCode(0x63, "LD H, E", [](ProcessorState &s) {
    s.registers.H = s.registers.E;
    return 4;
  });
  createOpCode(0x64, "LD H, H", [](ProcessorState &s) { return 4; });
  createOpCode(0x65, "LD H, L", [](ProcessorState &s) {
    s.registers.H = s.registers.L;
    return 4;
  });
  createOpCode(0x67, "LD H, A", [](ProcessorState &s) {
    s.registers.H = s.registers.A;
    return 4;
  });

  // LD L, r
  createOpCode(0x68, "LD L, B", [](ProcessorState &s) {
    s.registers.L = s.registers.B;
    return 4;
  });
  createOpCode(0x69, "LD L, C", [](ProcessorState &s) {
    s.registers.L = s.registers.C;
    return 4;
  });
  createOpCode(0x6A, "LD L, D", [](ProcessorState &s) {
    s.registers.L = s.registers.D;
    return 4;
  });
  createOpCode(0x6B, "LD L, E", [](ProcessorState &s) {
    s.registers.L = s.registers.E;
    return 4;
  });
  createOpCode(0x6C, "LD L, H", [](ProcessorState &s) {
    s.registers.L = s.registers.H;
    return 4;
  });
  createOpCode(0x6D, "LD L, L", [](ProcessorState &s) { return 4; });
  createOpCode(0x6F, "LD L, A", [](ProcessorState &s) {
    s.registers.L = s.registers.A;
    return 4;
  });

  // LD A, r
  createOpCode(0x78, "LD A, B", [](ProcessorState &s) {
    s.registers.A = s.registers.B;
    return 4;
  });
  createOpCode(0x79, "LD A, C", [](ProcessorState &s) {
    s.registers.A = s.registers.C;
    return 4;
  });
  createOpCode(0x7A, "LD A, D", [](ProcessorState &s) {
    s.registers.A = s.registers.D;
    return 4;
  });
  createOpCode(0x7B, "LD A, E", [](ProcessorState &s) {
    s.registers.A = s.registers.E;
    return 4;
  });
  createOpCode(0x7C, "LD A, H", [](ProcessorState &s) {
    s.registers.A = s.registers.H;
    return 4;
  });
  createOpCode(0x7D, "LD A, L", [](ProcessorState &s) {
    s.registers.A = s.registers.L;
    return 4;
  });
  createOpCode(0x7F, "LD A, A", [](ProcessorState &s) { return 4; });

  createOpCode(EX_AF_AF, "EX_AF_AF", processEX_AF_AF);
  createOpCode(EXX, "EXX", processEXX);
  createOpCode(EX_DE_HL, "EX_DE_HL", processEX_DE_HL);
  createOpCode(EX_SP_HL, "EX_SP_HL", processEX_SP_HL);

  // LD r, n (8-bit immediate)
  createOpCode(0x06, "LD B, n", [](ProcessorState &s) {
    s.registers.B = s.getNextByteFromPC();
    s.incPC();
    return 7;
  });
  createOpCode(0x0E, "LD C, n", [](ProcessorState &s) {
    s.registers.C = s.getNextByteFromPC();
    s.incPC();
    return 7;
  });
  createOpCode(0x16, "LD D, n", [](ProcessorState &s) {
    s.registers.D = s.getNextByteFromPC();
    s.incPC();
    return 7;
  });
  createOpCode(0x1E, "LD E, n", [](ProcessorState &s) {
    s.registers.E = s.getNextByteFromPC();
    s.incPC();
    return 7;
  });
  createOpCode(0x26, "LD H, n", [](ProcessorState &s) {
    s.registers.H = s.getNextByteFromPC();
    s.incPC();
    return 7;
  });
  createOpCode(0x2E, "LD L, n", [](ProcessorState &s) {
    s.registers.L = s.getNextByteFromPC();
    s.incPC();
    return 7;
  });
  createOpCode(0x3E, "LD A, n", [](ProcessorState &s) {
    s.registers.A = s.getNextByteFromPC();
    s.incPC();
    return 7;
    s.incPC();
    return 7;
  });

  // LD (BC), A (02)
  createOpCode(0x02, "LD (BC), A", [](ProcessorState &s) {
    s.memory[s.registers.BC] = s.registers.A;
    return 7;
  });
  // LD A, (BC) (0A)
  createOpCode(0x0A, "LD A, (BC)", [](ProcessorState &s) {
    s.registers.A = s.memory[s.registers.BC];
    return 7;
  });

  // LD (DE), A (12)
  createOpCode(0x12, "LD (DE), A", [](ProcessorState &s) {
    s.memory[s.registers.DE] = s.registers.A;
    return 7;
  });
  // LD A, (DE) (1A)
  createOpCode(0x1A, "LD A, (DE)", [](ProcessorState &s) {
    s.registers.A = s.memory[s.registers.DE];
    return 7;
  });
}

/**
 * Load a value into DE (word)
 * 10 t-states
 * @param state
 * @return
 */
int LoadOpcodes::processLD_DE_XX(ProcessorState &state) {
  // Get the next word and put it in DE
  word value = state.getNextWordFromPC();
  debug("LD DE,%#06x\n", value);
  state.registers.DE = value;
  state.incPC(2);
  return 10;
}

/**
 * Load the value of A into B
 * 4 t-states
 * @param state
 * @return
 */
int LoadOpcodes::processLD_B_A(ProcessorState &state) {
  state.registers.B = state.registers.A;
  debug("LD B, A (%#04x)\n", state.registers.B);
  return 4;
}

/**
 * Load the value of D into H
 * 4 t-states
 * @param state
 * @return
 */
int LoadOpcodes::processLD_H_D(ProcessorState &state) {
  state.registers.H = state.registers.D;
  debug("LD H, D (%#04x)\n", state.registers.H);
  return 4;
}

/**
 * Load the value of E into L
 * 4 t-states
 * @param state
 * @return
 */
int LoadOpcodes::processLD_L_E(ProcessorState &state) {
  state.registers.L = state.registers.E;
  debug("LD L, E (%#04x)\n", state.registers.L);
  return 4;
}

/**
 * Load n into (HL)
 * 10 t-states
 * @param state
 * @return
 */
int LoadOpcodes::processLD_HL_N(ProcessorState &state) {
  byte val = state.getNextByteFromPC();
  state.incPC();
  state.memory[state.registers.HL] = val;
  return 10;
}

int LoadOpcodes::processEX_AF_AF(ProcessorState &state) {
  word temp = state.registers.AF;
  state.registers.AF = state.registers.AF_;
  state.registers.AF_ = temp;
  return 4;
}

int LoadOpcodes::processEXX(ProcessorState &state) {
  word tempBC = state.registers.BC;
  word tempDE = state.registers.DE;
  word tempHL = state.registers.HL;

  state.registers.BC = state.registers.BC_;
  state.registers.DE = state.registers.DE_;
  state.registers.HL = state.registers.HL_;

  state.registers.BC_ = tempBC;
  state.registers.DE_ = tempDE;
  state.registers.HL_ = tempHL;

  return 4;
}

int LoadOpcodes::processEX_DE_HL(ProcessorState &state) {
  word temp = state.registers.DE;
  state.registers.DE = state.registers.HL;
  state.registers.HL = temp;
  return 4;
}

int LoadOpcodes::processEX_SP_HL(ProcessorState &state) {
  word tempL = state.memory[state.registers.SP];
  word tempH = state.memory[state.registers.SP + 1];
  word tempHL = (tempH << 8) | tempL;

  word oldHL = state.registers.HL;

  // Write old HL to stack
  state.memory[state.registers.SP] = (byte)(oldHL & 0xFF);
  state.memory[state.registers.SP + 1] = (byte)((oldHL >> 8) & 0xFF);

  state.registers.HL = tempHL;

  return 19;
}

int LoadOpcodes::processLD_NN_HL(ProcessorState &state) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();
  word address = (high << 8) | low;

  word val = state.registers.HL;
  state.memory[address] = (byte)(val & 0xFF);
  state.memory[address + 1] = (byte)((val >> 8) & 0xFF);
  return 16;
}

int LoadOpcodes::processLD_HL_NN(ProcessorState &state) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();
  word address = (high << 8) | low;

  byte valL = state.memory[address];
  byte valH = state.memory[address + 1];
  state.registers.HL = (valH << 8) | valL;
  return 16;
}

int LoadOpcodes::processLD_NN_A(ProcessorState &state) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();
  word address = (high << 8) | low;

  state.memory[address] = state.registers.A;
  return 13;
}

int LoadOpcodes::processLD_A_NN(ProcessorState &state) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();
  word address = (high << 8) | low;

  state.registers.A = state.memory[address];
  return 13;
}

int LoadOpcodes::processLD_SP_HL(ProcessorState &state) {
  state.registers.SP = state.registers.HL;
  return 6;
}

int LoadOpcodes::processLD_BC_NN(ProcessorState &state) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();
  state.registers.BC = (high << 8) | low;
  return 10;
}

int LoadOpcodes::processLD_DE_NN(ProcessorState &state) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();
  state.registers.DE = (high << 8) | low;
  return 10;
}

int LoadOpcodes::processLD_HL_NN_IMM(ProcessorState &state) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();
  state.registers.HL = (high << 8) | low;
  return 10;
}

int LoadOpcodes::processLD_SP_NN(ProcessorState &state) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();
  state.registers.SP = (high << 8) | low;
  return 10;
}

/**
 * Load the value of A into B
 * 4 t-states
 * @param state
 * @return
 */
int LoadOpcodes::processLD_A_X(ProcessorState &state) {
  byte value = state.getNextByteFromPC();
  state.registers.A = value;
  state.incPC();
  debug("LD A, %#04x\n", value);
  return 4;
}
