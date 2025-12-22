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

#ifndef ZXEMULATOR_EXTENDEDOPCODES_H
#define ZXEMULATOR_EXTENDEDOPCODES_H

#include "OpCodeProvider.h"

class ExtendedOpcodes : public OpCodeProvider {
public:
  ExtendedOpcodes();

  static int processExtended(ProcessorState &state);

  emulator_types::byte LDIR = 0xB0;
  emulator_types::byte CPIR = 0xB1;
  emulator_types::byte LDDR = 0xB8;
  emulator_types::byte OTDR = 0xBB;

  static int sbc16(ProcessorState &state, emulator_types::word val);
  static int adc16(ProcessorState &state, emulator_types::word val);
  static int ld_nn_rr(ProcessorState &state, emulator_types::word val);
  static int ld_rr_nn(ProcessorState &state, emulator_types::word &reg);

  static int processLDIR(ProcessorState &state);
  static int processCPIR(ProcessorState &state);
  static int processLDDR(ProcessorState &state);
};

#endif // ZXEMULATOR_EXTENDEDOPCODES_H
