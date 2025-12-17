// File: (ArithmeticOpcodes.h)
// Created by Antigravity

#ifndef ZXEMULATOR_ARITHMETICOPCODES_H
#define ZXEMULATOR_ARITHMETICOPCODES_H

#include "../../../utils/BaseTypes.h"
#include "../OpCode.h"
#include "../OpCodeProvider.h"

class ArithmeticOpcodes : public OpCodeProvider {
private:
  // ADD HL, rr
  static constexpr emulator_types::byte ADD_HL_BC = 0x09;
  static constexpr emulator_types::byte ADD_HL_DE = 0x19;
  static constexpr emulator_types::byte ADD_HL_HL = 0x29;
  static constexpr emulator_types::byte ADD_HL_SP = 0x39;

  // INC ss (16-bit)
  static constexpr emulator_types::byte INC_BC = 0x03;
  static constexpr emulator_types::byte INC_DE = 0x13;
  static constexpr emulator_types::byte INC_HL = 0x23;
  static constexpr emulator_types::byte INC_SP = 0x33;

  // INC r (8-bit)
  static constexpr emulator_types::byte INC_B = 0x04;
  static constexpr emulator_types::byte INC_C = 0x0C;
  static constexpr emulator_types::byte INC_D = 0x14;
  static constexpr emulator_types::byte INC_E = 0x1C;
  static constexpr emulator_types::byte INC_H = 0x24;
  static constexpr emulator_types::byte INC_L = 0x2C;
  static constexpr emulator_types::byte INC_HL_REF = 0x34;
  static constexpr emulator_types::byte INC_A = 0x3C;

  // DEC ss (16-bit)
  static constexpr emulator_types::byte DEC_BC = 0x0B;
  static constexpr emulator_types::byte DEC_DE = 0x1B;
  static constexpr emulator_types::byte DEC_HL = 0x2B;
  static constexpr emulator_types::byte DEC_SP = 0x3B;

  // DEC r (8-bit)
  static constexpr emulator_types::byte DEC_B = 0x05;
  static constexpr emulator_types::byte DEC_C = 0x0D;
  static constexpr emulator_types::byte DEC_D = 0x15;
  static constexpr emulator_types::byte DEC_E = 0x1D;
  static constexpr emulator_types::byte DEC_H = 0x25;
  static constexpr emulator_types::byte DEC_L = 0x2D;
  static constexpr emulator_types::byte DEC_HL_REF = 0x35;
  static constexpr emulator_types::byte DEC_A = 0x3D;

  // Helpers
  static int inc8(ProcessorState &state, emulator_types::byte &reg);
  static int dec8(ProcessorState &state, emulator_types::byte &reg);
  static int add8(ProcessorState &state, emulator_types::byte val);
  static int adc8(ProcessorState &state, emulator_types::byte val);
  static int sub8(ProcessorState &state, emulator_types::byte val);
  static int sbc8(ProcessorState &state, emulator_types::byte val);
  static int and8(ProcessorState &state, emulator_types::byte val);
  static int xor8(ProcessorState &state, emulator_types::byte val);
  static int or8(ProcessorState &state, emulator_types::byte val);
  static int cp8(ProcessorState &state, emulator_types::byte val);

  static int add16(ProcessorState &state, emulator_types::word &dest,
                   emulator_types::word src);
  static int inc16(ProcessorState &state, emulator_types::word &reg);
  static int dec16(ProcessorState &state, emulator_types::word &reg);

  // Handlers
  static int processADD_HL_BC(ProcessorState &state);
  static int processADD_HL_DE(ProcessorState &state);
  static int processADD_HL_HL(ProcessorState &state);
  static int processADD_HL_SP(ProcessorState &state);

  static int processDAA(ProcessorState &state);
  static int processCPL(ProcessorState &state);
  static int processSCF(ProcessorState &state);
  static int processCCF(ProcessorState &state);

  static int processINC_BC(ProcessorState &state);
  static int processINC_DE(ProcessorState &state);
  static int processINC_HL(ProcessorState &state);
  static int processINC_SP(ProcessorState &state);

  static int processINC_B(ProcessorState &state);
  static int processINC_C(ProcessorState &state);
  static int processINC_D(ProcessorState &state);
  static int processINC_E(ProcessorState &state);
  static int processINC_H(ProcessorState &state);
  static int processINC_L(ProcessorState &state);
  static int processINC_HL_REF(ProcessorState &state);

  static int processDEC_B(ProcessorState &state);
  static int processDEC_C(ProcessorState &state);
  static int processDEC_D(ProcessorState &state);
  static int processDEC_E(ProcessorState &state);
  static int processDEC_H(ProcessorState &state);
  static int processDEC_L(ProcessorState &state);
  static int processDEC_HL_REF(ProcessorState &state);

  static int processDEC_BC(ProcessorState &state);
  static int processDEC_DE(ProcessorState &state);
  static int processDEC_HL(ProcessorState &state);
  static int processDEC_SP(ProcessorState &state);

public:
  ArithmeticOpcodes();
};

#endif // ZXEMULATOR_ARITHMETICOPCODES_H
