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

#ifndef ZXEMULATOR_INDEXOPCODES_H
#define ZXEMULATOR_INDEXOPCODES_H

#include "../../utils/BaseTypes.h"
#include "../ProcessorState.h"
#include "OpCodeProvider.h"

class IndexOpcodes : public OpCodeProvider {
public:
  IndexOpcodes();

  static int processIX(ProcessorState &state);
  static int processIY(ProcessorState &state);

private:
  static int processIndex(ProcessorState &state,
                          emulator_types::word &indexReg);
  static int processIndexCB(ProcessorState &state,
                            emulator_types::word &indexReg);
  static int ld_r_idx(ProcessorState &state, emulator_types::word indexReg,
                      emulator_types::byte &reg);
  static int ld_idx_r(ProcessorState &state, emulator_types::word indexReg,
                      emulator_types::byte reg);
};

#endif // ZXEMULATOR_INDEXOPCODES_H
