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

#include "ProcessorState.h"

void ProcessorState::setInterrupts(bool value) {
  this->interruptsEnabled = value;
  this->registers.IFF1 = value ? 1 : 0;
  this->registers.IFF2 = value ? 1 : 0;
}

/**
 * Get the next word at the current program counter
 * @return
 */
word ProcessorState::getNextWordFromPC() {
  return this->memory.getWord(this->registers.PC);
}

/**
 * Get the next byte at the current program counter
 * @return
 */
byte ProcessorState::getNextByteFromPC() {
  return this->memory[this->registers.PC];
}

long ProcessorState::incPC(int value) {
  this->registers.PC += value;
  return this->registers.PC;
}

long ProcessorState::decPC(int value) {
  this->registers.PC -= value;
  return this->registers.PC;
}

long ProcessorState::incPC() { return incPC(1); }

long ProcessorState::setPC(long address) {
  return (this->registers.PC = address);
}
