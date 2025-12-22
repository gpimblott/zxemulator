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

#ifndef ZXEMULATOR_LOADOPCODES_H
#define ZXEMULATOR_LOADOPCODES_H

#include "../../../utils/BaseTypes.h"
#include "../OpCode.h"
#include "../OpCodeProvider.h"

class LoadOpcodes : public OpCodeProvider {
private:
  static constexpr emulator_types::byte LD_DE_XX = 0x11;
  static constexpr emulator_types::byte LD_HL_N = 0x36;
  static constexpr emulator_types::byte LD_B_A = 0x47;
  // Exchange
  static constexpr emulator_types::byte EX_AF_AF = 0x08;
  static constexpr emulator_types::byte EXX = 0xD9;
  static constexpr emulator_types::byte EX_DE_HL = 0xEB;
  static constexpr emulator_types::byte EX_SP_HL = 0xE3;
  static constexpr emulator_types::byte LD_SP_HL = 0xF9;
  // LD H, D
  static constexpr emulator_types::byte LD_H_D = 0x62;
  static constexpr emulator_types::byte LD_L_E = 0x6B;
  static constexpr emulator_types::byte LD_A_X = 0x3e;

  static constexpr emulator_types::byte LD_BC_NN = 0x01;
  static constexpr emulator_types::byte LD_DE_NN = 0x11;
  static constexpr emulator_types::byte LD_HL_NN_IMM = 0x21;
  static constexpr emulator_types::byte LD_SP_NN = 0x31;

  static constexpr emulator_types::byte LD_NN_HL = 0x22;
  static constexpr emulator_types::byte LD_HL_NN = 0x2A;
  static constexpr emulator_types::byte LD_NN_A = 0x32;
  static constexpr emulator_types::byte LD_A_NN = 0x3A;

  static int processLD_DE_XX(ProcessorState &state);
  static int processLD_HL_N(ProcessorState &state);
  static int processLD_B_A(ProcessorState &state);
  static int processLD_H_D(ProcessorState &state);
  static int processLD_L_E(ProcessorState &state);
  static int processLD_A_X(ProcessorState &state);

  static int processLD_BC_NN(ProcessorState &state);
  static int processLD_DE_NN(ProcessorState &state);
  static int processLD_HL_NN_IMM(ProcessorState &state);
  static int processLD_SP_NN(ProcessorState &state);

  static int processLD_NN_HL(ProcessorState &state);
  static int processLD_HL_NN(ProcessorState &state);
  static int processLD_NN_A(ProcessorState &state);
  static int processLD_A_NN(ProcessorState &state);
  static int processLD_SP_HL(ProcessorState &state);

  static int processEX_AF_AF(ProcessorState &state);
  static int processEXX(ProcessorState &state);
  static int processEX_DE_HL(ProcessorState &state);
  static int processEX_SP_HL(ProcessorState &state);

public:
  LoadOpcodes();
};

#endif // ZXEMULATOR_LOADOPCODES_H
