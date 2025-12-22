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
  bool paused = false;
  bool stepRequest = false;

  // Internal methods
  OpCode *getNextInstruction();

public:
  explicit Processor();

  OpCode *getOpCode(byte b) { return catalogue.lookupOpcode(b); }

  void init(const char *romFile);

  void run();
  void executeFrame();

  std::string lastError = "";

  VideoBuffer *getVideoBuffer();
  ProcessorState &getState() { return state; } // Expose for debugger
  bool isRunning() const { return running; }
  const std::string &getLastError() const { return lastError; }

  void shutdown();

  // Debug control
  void reset();
  void pause() { paused = true; }
  void resume() { paused = false; }
  void step() {
    if (paused)
      stepRequest = true;
  }
  bool isPaused() const { return paused; }
};

#endif // ZXEMULATOR_PROCESSOR_H
