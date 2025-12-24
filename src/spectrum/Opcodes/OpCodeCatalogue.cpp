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

#include "OpCodeCatalogue.h"
#include "ExtendedOpcodes.h"
#include "IndexOpcodes.h"

#include "instructions/BitOpcodes.h"
#include "instructions/IOOpcodes.h"
#include "instructions/InterruptOpcodes.h"
#include "instructions/JumpOpcodes.h"
#include "instructions/LoadOpcodes.h"

#include "instructions/RotateOpcodes.h"

/**
 * Constructor to build all the opcode instances and add to the catalogue
 */
OpCodeCatalogue::OpCodeCatalogue() {
  add(new LoadOpcodes());
  add(new InterruptOpcodes());

  add(new JumpOpcodes());
  add(new IOOpcodes());
  add(new ExtendedOpcodes());
  add(new IndexOpcodes());

  add(new BitOpcodes());

  add(new RotateOpcodes());

  // Build the O(1) lookup table
  for (int i = 0; i < 256; i++) {
    m_opcodeLookup[i] = nullptr;
    for (OpCodeProvider *provider : providersList) {
      OpCode *code = provider->lookupOpcode((byte)i);
      if (code != nullptr) {
        m_opcodeLookup[i] = code;
        break;
      }
    }
  }
}

OpCodeCatalogue::~OpCodeCatalogue() {
  for (OpCodeProvider *provider : providersList) {
    free(provider);
  }
}

/**
 * Add a new provider to the lookup list
 * @param opcode
 */
void OpCodeCatalogue::add(OpCodeProvider *provider) {
  providersList.push_back(provider);
};

/**
 * Find the specified opcode by asking each provider if it can process it
 *
 * NB : This is not optimal as we have to scan through each provider.  It would
 * be faster to build a master list of all opcodes and their functions
 *
 * @param opcode code to lookupOpcode
 * @return Either a pointer to the opcode class or null
 */
OpCode *OpCodeCatalogue::lookupOpcode(byte opcode) {
  return m_opcodeLookup[opcode];
}
