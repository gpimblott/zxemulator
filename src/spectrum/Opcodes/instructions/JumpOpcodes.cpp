// File: (JumpOpcodes.cpp)
// Created by G.Pimblott on 26/02/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "JumpOpcodes.h"
#include "../../../utils/debug.h"

JumpOpcodes::JumpOpcodes() : OpCodeProvider() {
  createOpCode(JP_XX, "JP_XX", processJP_XX);
  createOpCode(JR, "JR", processJR_E);     // Assuming JR maps to processJR_E
  createOpCode(DJNZ, "DJNZ", processDJNZ); // Assuming DJNZ is a new opcode with
                                           // its own process function

  createOpCode(0x20, "JR NZ, e", [](ProcessorState &s) {
    if (!GET_FLAG(Z_FLAG, s.registers)) {
      return processJR(s);
    }
    s.incPC();
    s.incPC();
    return 7;
  });
  createOpCode(0x28, "JR Z, e", [](ProcessorState &s) {
    if (GET_FLAG(Z_FLAG, s.registers)) {
      return processJR(s);
    }
    s.incPC();
    s.incPC();
    return 7;
  });
  createOpCode(0x30, "JR NC, e", [](ProcessorState &s) {
    if (!GET_FLAG(C_FLAG, s.registers)) {
      return processJR(s);
    }
    s.incPC();
    s.incPC();
    return 7;
  });
  createOpCode(JR_C, "JR C, e", [](ProcessorState &s) {
    if (GET_FLAG(C_FLAG, s.registers)) {
      return processJR(s);
    }
    s.incPC();
    s.incPC();
    return 7;
  });
  createOpCode(0xC3, "JP nn", processJP);
  createOpCode(RST_00, "RST 00",
               [](ProcessorState &s) { return processRST(s, 0x00); });
  createOpCode(RST_08, "RST 08",
               [](ProcessorState &s) { return processRST(s, 0x08); });
  createOpCode(RST_10, "RST 10",
               [](ProcessorState &s) { return processRST(s, 0x10); });
  createOpCode(RST_18, "RST 18",
               [](ProcessorState &s) { return processRST(s, 0x18); });
  createOpCode(RST_20, "RST 20",
               [](ProcessorState &s) { return processRST(s, 0x20); });
  createOpCode(RST_28, "RST 28",
               [](ProcessorState &s) { return processRST(s, 0x28); });
  createOpCode(RST_30, "RST 30",
               [](ProcessorState &s) { return processRST(s, 0x30); });
  createOpCode(RST_38, "RST 38",
               [](ProcessorState &s) { return processRST(s, 0x38); });

  // JP (HL) - 0xE9
  createOpCode(0xE9, "JP (HL)", [](ProcessorState &s) {
    s.setPC(s.registers.HL);
    return 4;
  });

  createOpCode(CALL_NN, "CALL nn", processCALL);
  createOpCode(RET, "RET", processRET);

  // Conditional RETs
  createOpCode(0xC0, "RET NZ", [](ProcessorState &s) {
    if (!GET_FLAG(Z_FLAG, s.registers))
      return processRET(s) + 6;
    return 5;
  });
  createOpCode(0xC8, "RET Z", [](ProcessorState &s) {
    if (GET_FLAG(Z_FLAG, s.registers))
      return processRET(s) + 6;
    return 5;
  });
  createOpCode(0xD0, "RET NC", [](ProcessorState &s) {
    if (!GET_FLAG(C_FLAG, s.registers))
      return processRET(s) + 6;
    return 5;
  });
  createOpCode(0xD8, "RET C", [](ProcessorState &s) {
    if (GET_FLAG(C_FLAG, s.registers))
      return processRET(s) + 6;
    return 5;
  });
  createOpCode(0xE0, "RET PO", [](ProcessorState &s) {
    if (!GET_FLAG(P_FLAG, s.registers))
      return processRET(s) + 6;
    return 5;
  });
  createOpCode(0xE8, "RET PE", [](ProcessorState &s) {
    if (GET_FLAG(P_FLAG, s.registers))
      return processRET(s) + 6;
    return 5;
  });
  createOpCode(0xF0, "RET P", [](ProcessorState &s) {
    if (!GET_FLAG(S_FLAG, s.registers))
      return processRET(s) + 6;
    return 5;
  });
  createOpCode(0xF8, "RET M", [](ProcessorState &s) {
    if (GET_FLAG(S_FLAG, s.registers))
      return processRET(s) + 6;
    return 5;
  });

  // Conditional CALLs
  createOpCode(0xC4, "CALL NZ, nn", [](ProcessorState &s) {
    if (!GET_FLAG(Z_FLAG, s.registers))
      return processCALL(s) + 7;
    s.incPC(2);
    return 10;
  });
  createOpCode(0xCC, "CALL Z, nn", [](ProcessorState &s) {
    if (GET_FLAG(Z_FLAG, s.registers))
      return processCALL(s) + 7;
    s.incPC(2);
    return 10;
  });
  createOpCode(0xD4, "CALL NC, nn", [](ProcessorState &s) {
    if (!GET_FLAG(C_FLAG, s.registers))
      return processCALL(s) + 7;
    s.incPC(2);
    return 10;
  });
  createOpCode(0xDC, "CALL C, nn", [](ProcessorState &s) {
    if (GET_FLAG(C_FLAG, s.registers))
      return processCALL(s) + 7;
    s.incPC(2);
    return 10;
  });
  createOpCode(0xE4, "CALL PO, nn", [](ProcessorState &s) {
    if (!GET_FLAG(P_FLAG, s.registers))
      return processCALL(s) + 7;
    s.incPC(2);
    return 10;
  });
  createOpCode(0xEC, "CALL PE, nn", [](ProcessorState &s) {
    if (GET_FLAG(P_FLAG, s.registers))
      return processCALL(s) + 7;
    s.incPC(2);
    return 10;
  });
  createOpCode(0xF4, "CALL P, nn", [](ProcessorState &s) {
    if (!GET_FLAG(S_FLAG, s.registers))
      return processCALL(s) + 7;
    s.incPC(2);
    return 10;
  });
  createOpCode(0xFC, "CALL M, nn", [](ProcessorState &s) {
    if (GET_FLAG(S_FLAG, s.registers))
      return processCALL(s) + 7;
    s.incPC(2);
    return 10;
  });

  // JP cc
  createOpCode(0xC2, "JP NZ, nn", [](ProcessorState &s) {
    if (!GET_FLAG(Z_FLAG, s.registers))
      return processJP(s);
    s.incPC(2);
    return 10;
  });
  createOpCode(0xCA, "JP Z, nn", [](ProcessorState &s) {
    if (GET_FLAG(Z_FLAG, s.registers))
      return processJP(s);
    s.incPC(2);
    return 10;
  });
  createOpCode(0xD2, "JP NC, nn", [](ProcessorState &s) {
    if (!GET_FLAG(C_FLAG, s.registers))
      return processJP(s);
    s.incPC(2);
    return 10;
  });
  createOpCode(0xDA, "JP C, nn", [](ProcessorState &s) {
    if (GET_FLAG(C_FLAG, s.registers))
      return processJP(s);
    s.incPC(2);
    return 10;
  });
  createOpCode(0xE2, "JP PO, nn", [](ProcessorState &s) {
    if (!GET_FLAG(P_FLAG, s.registers))
      return processJP(s);
    s.incPC(2);
    return 10;
  });
  createOpCode(0xEA, "JP PE, nn", [](ProcessorState &s) {
    if (GET_FLAG(P_FLAG, s.registers))
      return processJP(s);
    s.incPC(2);
    return 10;
  });
  createOpCode(0xF2, "JP P, nn", [](ProcessorState &s) {
    if (!GET_FLAG(S_FLAG, s.registers))
      return processJP(s);
    s.incPC(2);
    return 10;
  });
  createOpCode(0xFA, "JP M, nn", [](ProcessorState &s) {
    if (GET_FLAG(S_FLAG, s.registers))
      return processJP(s);
    s.incPC(2);
    return 10;
  });
}

/**
 * JP XX
 * 10 t-states
 * @param state
 * @return
 */
int JumpOpcodes::processJP_XX(ProcessorState &state) {
  word address = state.getNextWordFromPC();
  // debug("JP %#06x\n", address);
  state.setPC(address);
  return 10;
}

int JumpOpcodes::processJR_E(ProcessorState &state) {
  signed char offset = (signed char)state.getNextByteFromPC();
  state.incPC();

  state.incPC(offset);
  // debug("JR %d\n", offset);
  return 12;
}

int JumpOpcodes::processJR_NZ_E(ProcessorState &state) {
  signed char offset = (signed char)state.getNextByteFromPC();
  state.incPC();

  if (!(GET_FLAG(Z_FLAG, state.registers))) {
    state.incPC(offset);
    state.incPC(offset);
    // debug("JR NZ, %d (Taken)\n", offset);
    return 12;
  }
  // debug("JR NZ, %d (Not Taken)\n", offset);
  return 7;
}

int JumpOpcodes::processJR_Z_E(ProcessorState &state) {
  signed char offset = (signed char)state.getNextByteFromPC();
  state.incPC();

  if (GET_FLAG(Z_FLAG, state.registers)) {
    state.incPC(offset);
    state.incPC(offset);
    // debug("JR Z, %d (Taken)\n", offset);
    return 12;
  }
  // debug("JR Z, %d (Not Taken)\n", offset);
  return 7;
}

int JumpOpcodes::processJR_NC_E(ProcessorState &state) {
  signed char offset = (signed char)state.getNextByteFromPC();
  state.incPC();

  if (!(GET_FLAG(C_FLAG, state.registers))) {
    state.incPC(offset);
    state.incPC(offset);
    // debug("JR NC, %d (Taken)\n", offset);
    return 12;
  }
  // debug("JR NC, %d (Not Taken)\n", offset);
  return 7;
}

int JumpOpcodes::processRST(ProcessorState &state, int address) {
  // PUSH PC
  state.registers.SP -= 2;
  emulator_types::word pc = state.registers.PC;
  state.memory[state.registers.SP] = (byte)(pc & 0xFF);
  state.memory[state.registers.SP + 1] = (byte)((pc >> 8) & 0xFF);

  // Jump
  state.setPC(address);
  debug("RST %02X\n", address);
  return 11;
}

int JumpOpcodes::processDJNZ(ProcessorState &state) {
  signed char offset = (signed char)state.getNextByteFromPC();
  state.incPC();

  state.registers.B--;
  if (state.registers.B != 0) {
    state.incPC(offset);
    // debug("DJNZ %d (Taken)\n", offset);
    return 13;
  }
  // debug("DJNZ %d (Not Taken)\n", offset);
  return 8;
}

int JumpOpcodes::processJR(ProcessorState &state) { return processJR_E(state); }

int JumpOpcodes::processCALL(ProcessorState &state) {
  word address = state.getNextWordFromPC();
  debug("CALL %04X\n", address);

  // Return address is PC + 2 (skipping the 2 bytes of the address we just read)
  // Note: PC currently points to the low byte of the address.
  word returnAddress = state.registers.PC + 2;

  // PUSH Return Address
  state.registers.SP -= 2;
  state.memory[state.registers.SP] = (byte)(returnAddress & 0xFF);
  state.memory[state.registers.SP + 1] = (byte)((returnAddress >> 8) & 0xFF);

  state.setPC(address);
  return 17;
}

int JumpOpcodes::processRET(ProcessorState &state) {
  // POP PC
  byte low = state.memory[state.registers.SP];
  byte high = state.memory[state.registers.SP + 1];
  state.registers.SP += 2;

  word address = (high << 8) | low;
  state.setPC(address);
  debug("RET\n");
  return 10;
}

int JumpOpcodes::processJP(ProcessorState &state) {
  // JP nn
  // Read operand
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();

  word address = (high << 8) | low;
  state.registers.PC = address;
  return 10;
}
