// File: (StackOpcodes.h)
#ifndef ZXEMULATOR_STACKOPCODES_H
#define ZXEMULATOR_STACKOPCODES_H

#include "../../../utils/BaseTypes.h"
#include "../OpCodeProvider.h"

class StackOpcodes : public OpCodeProvider {
private:
  static const int PUSH_BC = 0xC5;
  static const int PUSH_DE = 0xD5;
  static const int PUSH_HL = 0xE5;
  static const int PUSH_AF = 0xF5;

  static const int POP_BC = 0xC1;
  static const int POP_DE = 0xD1;
  static const int POP_HL = 0xE1;
  static const int POP_AF = 0xF1;

  static int processPUSH_BC(ProcessorState &state);
  static int processPUSH_DE(ProcessorState &state);
  static int processPUSH_HL(ProcessorState &state);
  static int processPUSH_AF(ProcessorState &state);

  static int processPOP_BC(ProcessorState &state);
  static int processPOP_DE(ProcessorState &state);
  static int processPOP_HL(ProcessorState &state);
  static int processPOP_AF(ProcessorState &state);

  static void push(ProcessorState &state, emulator_types::word val);
  static emulator_types::word pop(ProcessorState &state);

public:
  StackOpcodes();
};

#endif // ZXEMULATOR_STACKOPCODES_H
