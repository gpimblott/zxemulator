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

#ifndef ZXEMULATOR_PROCESSORSTATE_H
#define ZXEMULATOR_PROCESSORSTATE_H

#include "Keyboard.h"
#include "Memory.h"
#include "ProcessorTypes.h"
#include "Tape.h"

class ProcessorState {
private:
  bool interruptsEnabled = false;
  int interruptMode = 0; // Default IM 0 on reset
  bool halted = false;
  bool speakerBit = false;
  bool micBit = false;
  long frameTStates = 0;
  bool fastLoad = false;

public:
  Z80Registers registers;
  Memory memory;
  Keyboard keyboard;
  Tape tape;

  // Supporting routines
  void setInterrupts(bool value);
  bool areInterruptsEnabled() const { return interruptsEnabled; }
  void setInterruptMode(int mode) { interruptMode = mode; }
  int getInterruptMode() const { return interruptMode; }
  void setHalted(bool value) { halted = value; }
  bool isHalted() const { return halted; }

  void setSpeakerBit(bool value) { speakerBit = value; }
  bool getSpeakerBit() const { return speakerBit; }
  void setMicBit(bool value) { micBit = value; }
  bool getMicBit() const { return micBit; }

  void setFrameTStates(long ts) { frameTStates = ts; }
  long getFrameTStates() const { return frameTStates; }
  void addFrameTStates(long ts) { frameTStates += ts; }

  void setFastLoad(bool value) { fastLoad = value; }
  bool isFastLoad() const { return fastLoad; }

  word getNextWordFromPC();
  byte getNextByteFromPC();

  // Program counter util functions
  long incPC();
  long incPC(int value);
  long decPC(int value);
  long setPC(long address);
};

#endif // ZXEMULATOR_PROCESSORSTATE_H
