#ifndef ZXEMULATOR_INDEXOPCODES_H
#define ZXEMULATOR_INDEXOPCODES_H

#include "../../utils/BaseTypes.h"
#include "../ProcessorState.h"
#include "OpCodeProvider.h"

class IndexOpcodes : public OpCodeProvider {
public:
  IndexOpcodes();

  static int processIX(ProcessorState &state);
  static int processIY(ProcessorState &state);

private:
  static int processIndex(ProcessorState &state,
                          emulator_types::word &indexReg);
  static int processIndexCB(ProcessorState &state,
                            emulator_types::word &indexReg);
  static int ld_r_idx(ProcessorState &state, emulator_types::word indexReg,
                      emulator_types::byte &reg);
  static int ld_idx_r(ProcessorState &state, emulator_types::word indexReg,
                      emulator_types::byte reg);
};

#endif // ZXEMULATOR_INDEXOPCODES_H
