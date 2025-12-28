/*
 * Copyright 2026 G.Pimblott
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
