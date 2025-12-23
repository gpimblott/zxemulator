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

#include "LoadOpcodes.h"
#include "../../../utils/debug.h"

/**
 * Add the opcodes to the catalogue
 * @param state
 * @param catalogue
 */
LoadOpcodes::LoadOpcodes() : OpCodeProvider() {

  // LD (HL), r

  // LD r, (HL)

  // LD r, r' (0x40 - 0x7F excluding HALT and (HL))
  // LD B, r

  createOpCode(EX_AF_AF, "EX_AF_AF", processEX_AF_AF);
  createOpCode(EXX, "EXX", processEXX);
  createOpCode(EX_DE_HL, "EX_DE_HL", processEX_DE_HL);
  createOpCode(EX_SP_HL, "EX_SP_HL", processEX_SP_HL);

  // LD r, n (8-bit immediate)

  // LD (BC), A (02)
}

/**
 * Load a value into DE (word)
 * 10 t-states
 * @param state
 * @return
 */
int LoadOpcodes::processLD_DE_XX(ProcessorState &state) {
  // Get the next word and put it in DE
  word value = state.getNextWordFromPC();
  debug("LD DE,%#06x\n", value);
  state.registers.DE = value;
  state.incPC(2);
  return 10;
}

/**
 * Load the value of A into B
 * 4 t-states
 * @param state
 * @return
 */
int LoadOpcodes::processLD_B_A(ProcessorState &state) {
  state.registers.B = state.registers.A;
  debug("LD B, A (%#04x)\n", state.registers.B);
  return 4;
}

/**
 * Load the value of D into H
 * 4 t-states
 * @param state
 * @return
 */
int LoadOpcodes::processLD_H_D(ProcessorState &state) {
  state.registers.H = state.registers.D;
  debug("LD H, D (%#04x)\n", state.registers.H);
  return 4;
}

/**
 * Load the value of E into L
 * 4 t-states
 * @param state
 * @return
 */
int LoadOpcodes::processLD_L_E(ProcessorState &state) {
  state.registers.L = state.registers.E;
  debug("LD L, E (%#04x)\n", state.registers.L);
  return 4;
}

/**
 * Load n into (HL)
 * 10 t-states
 * @param state
 * @return
 */
int LoadOpcodes::processLD_HL_N(ProcessorState &state) {
  byte val = state.getNextByteFromPC();
  state.incPC();
  state.memory[state.registers.HL] = val;
  return 10;
}

int LoadOpcodes::processEX_AF_AF(ProcessorState &state) {
  word temp = state.registers.AF;
  state.registers.AF = state.registers.AF_;
  state.registers.AF_ = temp;
  return 4;
}

int LoadOpcodes::processEXX(ProcessorState &state) {
  word tempBC = state.registers.BC;
  word tempDE = state.registers.DE;
  word tempHL = state.registers.HL;

  state.registers.BC = state.registers.BC_;
  state.registers.DE = state.registers.DE_;
  state.registers.HL = state.registers.HL_;

  state.registers.BC_ = tempBC;
  state.registers.DE_ = tempDE;
  state.registers.HL_ = tempHL;

  return 4;
}

int LoadOpcodes::processEX_DE_HL(ProcessorState &state) {
  word temp = state.registers.DE;
  state.registers.DE = state.registers.HL;
  state.registers.HL = temp;
  return 4;
}

int LoadOpcodes::processEX_SP_HL(ProcessorState &state) {
  word tempL = state.memory[state.registers.SP];
  word tempH = state.memory[state.registers.SP + 1];
  word tempHL = (tempH << 8) | tempL;

  word oldHL = state.registers.HL;

  // Write old HL to stack
  state.memory[state.registers.SP] = (byte)(oldHL & 0xFF);
  state.memory[state.registers.SP + 1] = (byte)((oldHL >> 8) & 0xFF);

  state.registers.HL = tempHL;

  return 19;
}

int LoadOpcodes::processLD_NN_HL(ProcessorState &state) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();
  word address = (high << 8) | low;

  word val = state.registers.HL;
  state.memory[address] = (byte)(val & 0xFF);
  state.memory[address + 1] = (byte)((val >> 8) & 0xFF);
  return 16;
}

int LoadOpcodes::processLD_HL_NN(ProcessorState &state) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();
  word address = (high << 8) | low;

  byte valL = state.memory[address];
  byte valH = state.memory[address + 1];
  state.registers.HL = (valH << 8) | valL;
  return 16;
}

int LoadOpcodes::processLD_NN_A(ProcessorState &state) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();
  word address = (high << 8) | low;

  state.memory[address] = state.registers.A;
  return 13;
}

int LoadOpcodes::processLD_A_NN(ProcessorState &state) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();
  word address = (high << 8) | low;

  state.registers.A = state.memory[address];
  return 13;
}

int LoadOpcodes::processLD_SP_HL(ProcessorState &state) {
  state.registers.SP = state.registers.HL;
  return 6;
}

int LoadOpcodes::processLD_BC_NN(ProcessorState &state) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();
  state.registers.BC = (high << 8) | low;
  return 10;
}

int LoadOpcodes::processLD_DE_NN(ProcessorState &state) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();
  state.registers.DE = (high << 8) | low;
  return 10;
}

int LoadOpcodes::processLD_HL_NN_IMM(ProcessorState &state) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();
  state.registers.HL = (high << 8) | low;
  return 10;
}

int LoadOpcodes::processLD_SP_NN(ProcessorState &state) {
  byte low = state.getNextByteFromPC();
  state.incPC();
  byte high = state.getNextByteFromPC();
  state.incPC();
  state.registers.SP = (high << 8) | low;
  return 10;
}

/**
 * Load the value of A into B
 * 4 t-states
 * @param state
 * @return
 */
int LoadOpcodes::processLD_A_X(ProcessorState &state) {
  byte value = state.getNextByteFromPC();
  state.registers.A = value;
  state.incPC();
  debug("LD A, %#04x\n", value);
  return 4;
}
