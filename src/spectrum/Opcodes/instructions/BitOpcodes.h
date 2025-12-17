// File: (BitOpcodes.h)
// Created by Antigravity

#ifndef ZXEMULATOR_BITOPCODES_H
#define ZXEMULATOR_BITOPCODES_H

#include "../../../utils/BaseTypes.h"
#include "../OpCode.h"
#include "../OpCodeProvider.h"

class BitOpcodes : public OpCodeProvider {
private:
  static int processCB(ProcessorState &state);

  // Helpers
  // Operations on reference (register or memory)
  static void rlc(ProcessorState &state, emulator_types::byte &val);
  static void rrc(ProcessorState &state, emulator_types::byte &val);
  static void rl(ProcessorState &state, emulator_types::byte &val);
  static void rr(ProcessorState &state, emulator_types::byte &val);
  static void sla(ProcessorState &state, emulator_types::byte &val);
  static void sra(ProcessorState &state, emulator_types::byte &val);
  static void srl(ProcessorState &state, emulator_types::byte &val);

  static void bit(ProcessorState &state, int bit, emulator_types::byte val);
  static void set(int bit, emulator_types::byte &val);
  static void res(int bit, emulator_types::byte &val);

public:
  BitOpcodes();
};

#endif // ZXEMULATOR_BITOPCODES_H
