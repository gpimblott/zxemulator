// File: (ProcessorState.h)
// Created by G.Pimblott on 26/02/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_PROCESSORSTATE_H
#define ZXEMULATOR_PROCESSORSTATE_H

#include "Memory.h"
#include "ProcessorTypes.h"

class ProcessorState {
private:
  bool interruptsEnabled = false;
  bool halted = false;

public:
  Z80Registers registers;
  Memory memory;

  // Supporting routines
  void setInterrupts(bool value);
  bool areInterruptsEnabled() const { return interruptsEnabled; }
  void setHalted(bool value) { halted = value; }
  bool isHalted() const { return halted; }
  word getNextWordFromPC();
  byte getNextByteFromPC();

  // Program counter util functions
  long incPC();
  long incPC(int value);
  long decPC(int value);
  long setPC(long address);
};

#endif // ZXEMULATOR_PROCESSORSTATE_H
