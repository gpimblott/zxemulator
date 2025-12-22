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

#ifndef ZXEMULATOR_OPCODE_H
#define ZXEMULATOR_OPCODE_H

#include "../../utils/BaseTypes.h"
#include "../ProcessorState.h"
#include "../ProcessorTypes.h"
#include <string>

// Define the type of the execution function
typedef int (*executeFunc_t)(ProcessorState &);

/**
 * Representation of a single instruction opcode recognised by the processor
 * This base class is extended by each specific instruction
 */
class OpCode {
private:
  emulator_types::byte code;
  std::string name;
  executeFunc_t executeFunc;

public:
  // Constructor
  OpCode(emulator_types::byte opcode, std::string name, executeFunc_t func);

  // Method that implements the specific code for an instruction
  int execute(ProcessorState &state);

  // Methods
  emulator_types::byte getOpCode();

  std::string getName();
};

#endif // ZXEMULATOR_OPCODE_H
