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

#ifndef ZXEMULATOR_JUMPOPCODES_H
#define ZXEMULATOR_JUMPOPCODES_H

#include "../../../utils/BaseTypes.h"
#include "../OpCodeProvider.h"

class JumpOpcodes : public OpCodeProvider {
private:
  static constexpr emulator_types::byte JP_XX = 0xc3;
  static constexpr emulator_types::byte JR_E = 0x18;
  static constexpr emulator_types::byte JR_NZ_E = 0x20;
  static constexpr emulator_types::byte JR_Z_E = 0x28;
  static constexpr emulator_types::byte JR_NC_E = 0x30;
  static constexpr emulator_types::byte JR_C_E = 0x38;
  static const int JR_NZ = 0x20;
  static const int JR_Z = 0x28;
  static const int JR_NC = 0x30;
  static const int JR_C = 0x38;
  static const int JR = 0x18;
  static const int DJNZ = 0x10;

  // RST
  static const int RST_00 = 0xC7;
  static const int RST_08 = 0xCF;
  static const int RST_10 = 0xD7;
  static const int RST_18 = 0xDF;
  static const int RST_20 = 0xE7;
  static const int RST_28 = 0xEF;
  static const int RST_30 = 0xF7;
  static const int RST_38 = 0xFF;

  // CALL
  static const int CALL_NN = 0xCD;
  // RET
  static const int RET = 0xC9;

  static int processJP_XX(ProcessorState &state);
  static int processJR_E(ProcessorState &state);
  static int processJR_NZ_E(ProcessorState &state);
  static int processJR_Z_E(ProcessorState &state);
  static int processJR_NC_E(ProcessorState &state);
  static int processJR_C_E(ProcessorState &state);
  static int processJR(ProcessorState &state);
  static int processDJNZ(ProcessorState &state);
  static int processRST(ProcessorState &state, int address);
  static int processCALL(ProcessorState &state);
  static int processRET(ProcessorState &state);
  static int processJP(ProcessorState &state); // Added helper

public:
  JumpOpcodes();
};

#endif // ZXEMULATOR_JUMPOPCODES_H
