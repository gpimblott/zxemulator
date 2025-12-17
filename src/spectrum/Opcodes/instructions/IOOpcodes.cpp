// File: (OutOpcodes.cpp)
// Created by G.Pimblott on 26/02/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "IOOpcodes.h"
#include "../../../utils/debug.h"

IOOpcodes::IOOpcodes() : OpCodeProvider() {
  createOpCode(OUT, "OUT", processOUT);
  createOpCode(IN_A_N, "IN A, (n)", processIN_A_N);
}

int IOOpcodes::processOUT(ProcessorState &state) {
  byte port = state.getNextByteFromPC();
  state.incPC();
  debug("OUT (%02X), A", port);
  // TODO: Implement actual IO
  return 11;
}

int IOOpcodes::processIN_A_N(ProcessorState &state) {
  byte port = state.getNextByteFromPC();
  state.incPC();
  debug("IN A, (%02X)", port);

  // TODO: Actual Input from Keyboard/Port
  // For now return 0xFF (floating bus)
  state.registers.A = 0xFF;

  return 11;
}
