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

#ifndef ZXEMULATOR_LOGICOPCODES_H
#define ZXEMULATOR_LOGICOPCODES_H

#include "../../../utils/BaseTypes.h"
#include "../OpCode.h"
#include "../OpCodeProvider.h"

class LogicOpcodes : public OpCodeProvider {
private:
  emulator_types::byte XOR_A = 0xaf;
  emulator_types::byte CP_N = 0xfe;
  emulator_types::byte NOP = 0x00;

  // CP r
  emulator_types::byte CP_B = 0xB8;
  emulator_types::byte CP_C = 0xB9;
  emulator_types::byte CP_D = 0xBA;
  emulator_types::byte CP_E = 0xBB;
  emulator_types::byte CP_H = 0xBC;
  emulator_types::byte CP_L = 0xBD;
  emulator_types::byte CP_HL = 0xBE;
  emulator_types::byte CP_A = 0xBF;

  // AND r
  emulator_types::byte AND_B = 0xA0;
  emulator_types::byte AND_C = 0xA1;
  emulator_types::byte AND_D = 0xA2;
  emulator_types::byte AND_E = 0xA3;
  emulator_types::byte AND_H = 0xA4;
  emulator_types::byte AND_L = 0xA5;
  emulator_types::byte AND_HL = 0xA6;
  emulator_types::byte AND_A = 0xA7;

  // XOR r (XOR A is 0xAF)
  emulator_types::byte XOR_B = 0xA8;
  emulator_types::byte XOR_C = 0xA9;
  emulator_types::byte XOR_D = 0xAA;
  emulator_types::byte XOR_E = 0xAB;
  emulator_types::byte XOR_H = 0xAC;
  emulator_types::byte XOR_L = 0xAD;
  emulator_types::byte XOR_HL = 0xAE;

  // OR r
  emulator_types::byte OR_B = 0xB0;
  emulator_types::byte OR_C = 0xB1;
  emulator_types::byte OR_D = 0xB2;
  emulator_types::byte OR_E = 0xB3;
  emulator_types::byte OR_H = 0xB4;
  emulator_types::byte OR_L = 0xB5;
  emulator_types::byte OR_HL = 0xB6;
  emulator_types::byte OR_A = 0xB7;

  // Helpers
public:
  static int cp8(ProcessorState &state, emulator_types::byte value);
  static int and8(ProcessorState &state, emulator_types::byte value);
  static int xor8(ProcessorState &state, emulator_types::byte value);
  static int or8(ProcessorState &state, emulator_types::byte value);

private:
  static int processXOR_A(ProcessorState &state);
  static int processCP_N(ProcessorState &state);
  static int processNOP(ProcessorState &state);

  static int processCP_B(ProcessorState &state);
  static int processCP_C(ProcessorState &state);
  static int processCP_D(ProcessorState &state);
  static int processCP_E(ProcessorState &state);
  static int processCP_H(ProcessorState &state);
  static int processCP_L(ProcessorState &state);
  static int processCP_HL(ProcessorState &state);
  static int processCP_A(ProcessorState &state);

  static int processAND_B(ProcessorState &state);
  static int processAND_C(ProcessorState &state);
  static int processAND_D(ProcessorState &state);
  static int processAND_E(ProcessorState &state);
  static int processAND_H(ProcessorState &state);
  static int processAND_L(ProcessorState &state);
  static int processAND_HL(ProcessorState &state);
  static int processAND_A(ProcessorState &state);

  static int processXOR_B(ProcessorState &state);
  static int processXOR_C(ProcessorState &state);
  static int processXOR_D(ProcessorState &state);
  static int processXOR_E(ProcessorState &state);
  static int processXOR_H(ProcessorState &state);
  static int processXOR_L(ProcessorState &state);
  static int processXOR_HL(ProcessorState &state);

  static int processOR_B(ProcessorState &state);
  static int processOR_C(ProcessorState &state);
  static int processOR_D(ProcessorState &state);
  static int processOR_E(ProcessorState &state);
  static int processOR_H(ProcessorState &state);
  static int processOR_L(ProcessorState &state);
  static int processOR_HL(ProcessorState &state);
  static int processOR_A(ProcessorState &state);

public:
  LogicOpcodes();
};

#endif // ZXEMULATOR_LOGICOPCODES_H
