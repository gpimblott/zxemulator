// File: (OutOpcodes.h)
// Created by G.Pimblott on 26/02/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_IOOPCODES_H
#define ZXEMULATOR_IOOPCODES_H

#include "../../../utils/BaseTypes.h"
#include "../OpCodeProvider.h"

class IOOpcodes : public OpCodeProvider {
private:
  static constexpr emulator_types::byte OUT = 0xD3;
  static constexpr emulator_types::byte IN_A_N = 0xDB;

  static int processOUT(ProcessorState &state);
  static int processIN_A_N(ProcessorState &state);

public:
  IOOpcodes();
};

#endif // ZXEMULATOR_IOOPCODES_H
