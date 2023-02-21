// File: (ProcessorState.h)
// Created by G.Pimblott on 19/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_PROCESSORTYPES_H
#define ZXEMULATOR_PROCESSORTYPES_H

#include "Memory.h"
#include "../utils/BaseTypes.h"

/**
 * Representation of the Z80 registers
 * The Z80 is little endian so the LSB is stored first
 */
struct Z80Registers {
    emulator_types::byte A;
    emulator_types::byte F;
    emulator_types::word *AF = (emulator_types::word*)(this + offsetof( struct Z80Registers, A));

    emulator_types::byte B;
    emulator_types::byte C;
    emulator_types::word *BC = (emulator_types::word*)(this + offsetof( struct Z80Registers, B));

    emulator_types::byte D;
    emulator_types::byte E;
    emulator_types::word *DE = (emulator_types::word*)(this + offsetof( struct Z80Registers, D));

    emulator_types::byte H;
    emulator_types::byte L;
    emulator_types::word *HL = (emulator_types::word*)(this + offsetof( struct Z80Registers, H));


    emulator_types::word pc;    // Program counter
    emulator_types::word sp;    // stack pointer
    emulator_types::byte flags; // flags register
};

/**
 * Wrapper for the core processor data
 */
struct ProcessorState{
    Z80Registers registers;
    Memory memory;
};

#endif //ZXEMULATOR_PROCESSORTYPES_H
