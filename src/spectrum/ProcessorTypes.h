/*
 * Copyright 2026 G.Pimblott
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

  // emulator_types::byte flags; // Deprecated. Use F in AF.
  emulator_types::byte I; // Interrupt Vector
  emulator_types::byte R; // Refresh Register

  emulator_types::byte IFF1; // Interrupt Flip-Flop 1
  emulator_types::byte IFF2; // Interrupt Flip-Flop 2
};

#endif // ZXEMULATOR_PROCESSORTYPES_H
