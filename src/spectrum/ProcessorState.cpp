// File: (ProcessorState.cpp)
// Created by G.Pimblott on 26/02/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "ProcessorState.h"

void ProcessorState::setInterrupts(bool value) {
  this->interruptsEnabled = value;
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
