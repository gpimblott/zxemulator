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

#ifndef ZXEMULATOR_PROCESSORTYPES_H
#define ZXEMULATOR_PROCESSORTYPES_H

#include "../utils/BaseTypes.h"
#include "Memory.h"
#include "ProcessorMacros.h"

/**
 * Representation of the Z80 registers
 * The Z80 is little endian so the LSB is stored first
 */
struct Z80Registers {
  union {
    struct {
      emulator_types::byte F;
      emulator_types::byte A;
    };
    emulator_types::word AF;
  };

  union {
    struct {
      emulator_types::byte C;
      emulator_types::byte B;
    };
    emulator_types::word BC;
  };

  union {
    struct {
      emulator_types::byte E;
      emulator_types::byte D;
    };
    emulator_types::word DE;
  };

  union {
    struct {
      emulator_types::byte L;
      emulator_types::byte H;
    };
    emulator_types::word HL;
  };

  emulator_types::word PC; // Program counter
  emulator_types::word SP; // stack pointer

  // Shadow Registers
  emulator_types::word AF_;
  emulator_types::word BC_;
  emulator_types::word DE_;
  emulator_types::word HL_;

  emulator_types::word IX; // Index Register X
  emulator_types::word IY; // Index Register Y

  emulator_types::byte flags; // flags register (Deprecated? Use F in AF)
  emulator_types::byte I;     // Interrupt Vector
  emulator_types::byte R;     // Refresh Register
};

#endif // ZXEMULATOR_PROCESSORTYPES_H
