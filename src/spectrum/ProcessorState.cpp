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
