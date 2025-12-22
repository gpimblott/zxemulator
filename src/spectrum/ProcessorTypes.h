// File: (ProcessorState.h)
// Created by G.Pimblott on 19/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

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
