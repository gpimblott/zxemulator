// File: (Processor.h)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_PROCESSOR_H
#define ZXEMULATOR_PROCESSOR_H

#include "Opcodes/OpCodeCatalogue.h"
#include "ProcessorTypes.h"

class Processor {

private:
  // State variables
  ProcessorState state;
  OpCodeCatalogue catalogue = OpCodeCatalogue();

  bool running = false;

  // Internal methods
  OpCode *getNextInstruction();

public:
  explicit Processor();

  void init(const char *romFile);

  void run();
  void executeFrame();

  VideoBuffer *getVideoBuffer();

  void shutdown();
};

#endif // ZXEMULATOR_PROCESSOR_H
