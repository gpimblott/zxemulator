#ifndef ZXEMULATOR_ALUHELPERS_H
#define ZXEMULATOR_ALUHELPERS_H

#include "ProcessorMacros.h"
#include "ProcessorState.h"
#include <cstdint>

/**
 * Shared ALU (Arithmetic Logic Unit) helper functions
 * Ensures consistent flag handling across all opcode implementations
 */
namespace ALUHelpers {

// 8-bit Arithmetic
void add8(ProcessorState &state, emulator_types::byte val);
void sub8(ProcessorState &state, emulator_types::byte val);
void adc8(ProcessorState &state, emulator_types::byte val);
void sbc8(ProcessorState &state, emulator_types::byte val);
void inc8(ProcessorState &state, emulator_types::byte &reg);
void dec8(ProcessorState &state, emulator_types::byte &reg);

// 8-bit Logic
void and8(ProcessorState &state, emulator_types::byte val);
void or8(ProcessorState &state, emulator_types::byte val);
void xor8(ProcessorState &state, emulator_types::byte val);
void cp8(ProcessorState &state, emulator_types::byte val);

// 16-bit Arithmetic
int add16(ProcessorState &state, emulator_types::word &dest,
          emulator_types::word src);
int inc16(ProcessorState &state, emulator_types::word &reg);
int dec16(ProcessorState &state, emulator_types::word &reg);

} // namespace ALUHelpers

#endif // ZXEMULATOR_ALUHELPERS_H
