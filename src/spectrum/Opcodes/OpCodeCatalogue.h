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

#ifndef ZXEMULATOR_OPCODECATALOGUE_H
#define ZXEMULATOR_OPCODECATALOGUE_H

#include "../../utils/BaseTypes.h"
#include "../ProcessorState.h"
#include "OpCode.h"
#include "OpCodeProvider.h"
#include "instructions/LoadOpcodes.h"
#include <list>

typedef std::list<OpCodeProvider *> providerList_t;

class OpCodeCatalogue {
private:
  providerList_t providersList;
  OpCode *m_opcodeLookup[256];

public:
  OpCodeCatalogue();

  void add(OpCodeProvider *provider);

  OpCode *lookupOpcode(byte opcode);

  virtual ~OpCodeCatalogue();
};

#endif // ZXEMULATOR_OPCODECATALOGUE_H
