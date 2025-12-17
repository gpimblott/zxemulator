// File: (LoadOpcodes.h)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_LOADOPCODES_H
#define ZXEMULATOR_LOADOPCODES_H

#include "../../../utils/BaseTypes.h"
#include "../OpCode.h"
#include "../OpCodeProvider.h"

class LoadOpcodes : public OpCodeProvider {
private:
  static constexpr emulator_types::byte LD_DE_XX = 0x11;
  static constexpr emulator_types::byte LD_HL_N = 0x36;
  static constexpr emulator_types::byte LD_B_A = 0x47;
  // Exchange
  static constexpr emulator_types::byte EX_AF_AF = 0x08;
  static constexpr emulator_types::byte EXX = 0xD9;
  static constexpr emulator_types::byte EX_DE_HL = 0xEB;
  static constexpr emulator_types::byte EX_SP_HL = 0xE3;
  static constexpr emulator_types::byte LD_SP_HL = 0xF9;
  // LD H, D
  static constexpr emulator_types::byte LD_H_D = 0x62;
  static constexpr emulator_types::byte LD_L_E = 0x6B;
  static constexpr emulator_types::byte LD_A_X = 0x3e;

  static constexpr emulator_types::byte LD_BC_NN = 0x01;
  static constexpr emulator_types::byte LD_DE_NN = 0x11;
  static constexpr emulator_types::byte LD_HL_NN_IMM = 0x21;
  static constexpr emulator_types::byte LD_SP_NN = 0x31;

  static constexpr emulator_types::byte LD_NN_HL = 0x22;
  static constexpr emulator_types::byte LD_HL_NN = 0x2A;
  static constexpr emulator_types::byte LD_NN_A = 0x32;
  static constexpr emulator_types::byte LD_A_NN = 0x3A;

  static int processLD_DE_XX(ProcessorState &state);
  static int processLD_HL_N(ProcessorState &state);
  static int processLD_B_A(ProcessorState &state);
  static int processLD_H_D(ProcessorState &state);
  static int processLD_L_E(ProcessorState &state);
  static int processLD_A_X(ProcessorState &state);

  static int processLD_BC_NN(ProcessorState &state);
  static int processLD_DE_NN(ProcessorState &state);
  static int processLD_HL_NN_IMM(ProcessorState &state);
  static int processLD_SP_NN(ProcessorState &state);

  static int processLD_NN_HL(ProcessorState &state);
  static int processLD_HL_NN(ProcessorState &state);
  static int processLD_NN_A(ProcessorState &state);
  static int processLD_A_NN(ProcessorState &state);
  static int processLD_SP_HL(ProcessorState &state);

  static int processEX_AF_AF(ProcessorState &state);
  static int processEXX(ProcessorState &state);
  static int processEX_DE_HL(ProcessorState &state);
  static int processEX_SP_HL(ProcessorState &state);

public:
  LoadOpcodes();
};

#endif // ZXEMULATOR_LOADOPCODES_H
