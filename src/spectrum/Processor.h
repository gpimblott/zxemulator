/*
 * MIT License
 *
 * Copyright (c) 2026 G.Pimblott
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef ZXEMULATOR_PROCESSOR_H
#define ZXEMULATOR_PROCESSOR_H

#include "Opcodes/OpCodeCatalogue.h"
#include "ProcessorTypes.h"

#include "Audio.h"

class Processor {

private:
  // State variables
  ProcessorState state;
  Audio audio;
  OpCodeCatalogue catalogue = OpCodeCatalogue();

  bool running = false;
  bool paused = false;
  bool stepRequest = false;

  // Auto-Load
  bool autoLoadTape = false;
  long frameCounter = 0;
  int autoLoadStep = 0;
  int keyHoldFrames = 0;

  // Internal methods
  OpCode *getNextInstruction();

  // Fast direct memory access
  byte *m_memory = nullptr;

  // Helper methods for instruction groups
  void op_load(byte opcode);
  void op_arithmetic(byte opcode);
  void op_logic(byte opcode);
  void op_rotate_shift(byte opcode);
  void op_bit(byte opcode);
  void op_jump(byte opcode);
  void op_stack(byte opcode);
  void op_io(byte opcode);
  void op_misc(byte opcode);

  // Extended instruction handlers
  void exec_ed_opcode();
  void exec_cb_opcode();
  void exec_index_opcode(byte prefix); // DD or FD

public:
  explicit Processor();

  OpCode *getOpCode(byte b) { return catalogue.lookupOpcode(b); }

  void init(const char *romFile);
  void loadTape(const char *filename);
  void loadSnapshot(const char *filename);
  void loadZ80Snapshot(const char *filename);

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
