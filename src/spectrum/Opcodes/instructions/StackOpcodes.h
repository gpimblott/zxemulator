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
#ifndef ZXEMULATOR_STACKOPCODES_H
#define ZXEMULATOR_STACKOPCODES_H

#include "../../../utils/BaseTypes.h"
#include "../OpCodeProvider.h"

class StackOpcodes : public OpCodeProvider {
private:
  static const int PUSH_BC = 0xC5;
  static const int PUSH_DE = 0xD5;
  static const int PUSH_HL = 0xE5;
  static const int PUSH_AF = 0xF5;

  static const int POP_BC = 0xC1;
  static const int POP_DE = 0xD1;
  static const int POP_HL = 0xE1;
  static const int POP_AF = 0xF1;

  static int processPUSH_BC(ProcessorState &state);
  static int processPUSH_DE(ProcessorState &state);
  static int processPUSH_HL(ProcessorState &state);
  static int processPUSH_AF(ProcessorState &state);

  static int processPOP_BC(ProcessorState &state);
  static int processPOP_DE(ProcessorState &state);
  static int processPOP_HL(ProcessorState &state);
  static int processPOP_AF(ProcessorState &state);

  static void push(ProcessorState &state, emulator_types::word val);
  static emulator_types::word pop(ProcessorState &state);

public:
  StackOpcodes();
};

#endif // ZXEMULATOR_STACKOPCODES_H
