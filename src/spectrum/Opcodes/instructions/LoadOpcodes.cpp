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

/**
 * Add the opcodes to the catalogue
 * @param state
 * @param catalogue
 */
LoadOpcodes::LoadOpcodes() : OpCodeProvider() {

  // Removed duplicates handled by Processor.cpp
  // (EX_DE_HL, EX_SP_HL, LD operations)

  createOpCode(EX_AF_AF, "EX_AF_AF", processEX_AF_AF);
  createOpCode(EXX, "EXX", processEXX);
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
