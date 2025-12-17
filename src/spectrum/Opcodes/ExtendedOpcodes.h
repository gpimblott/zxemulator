// File: (ExtendedOpcodes.h)
// Created by Antigravity

#ifndef ZXEMULATOR_EXTENDEDOPCODES_H
#define ZXEMULATOR_EXTENDEDOPCODES_H

#include "OpCodeProvider.h"

class ExtendedOpcodes : public OpCodeProvider {
public:
  ExtendedOpcodes();

  static int processExtended(ProcessorState &state);

  emulator_types::byte LDIR = 0xB0;
  emulator_types::byte CPIR = 0xB1;
  emulator_types::byte LDDR = 0xB8;
  emulator_types::byte OTDR = 0xBB;

  static int sbc16(ProcessorState &state, emulator_types::word val);
  static int adc16(ProcessorState &state, emulator_types::word val);
  static int ld_nn_rr(ProcessorState &state, emulator_types::word val);
  static int ld_rr_nn(ProcessorState &state, emulator_types::word &reg);

  static int processLDIR(ProcessorState &state);
  static int processCPIR(ProcessorState &state);
  static int processLDDR(ProcessorState &state);
};

#endif // ZXEMULATOR_EXTENDEDOPCODES_H
