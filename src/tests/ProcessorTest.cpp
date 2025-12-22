// File: (ProcessorTest.cpp)
// Created by G.Pimblott on 25/02/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "../spectrum/ProcessorTypes.h"
#include <gtest/gtest.h>

/**
 * Test the operation of the processor emulation
 * The Z80 is little endian so will write the least significant byte first
 */
TEST(ProcessorTest, Check8And16BitValues) {
  Z80Registers registers;
  byte byteH = 0xE5;     // 229
  byte byteL = 0x5A;     // 90
  word wordOne = 0x5AE5; // 23268

  // Set the 8 bit registers
  registers.H = byteH;
  registers.L = byteL;

  // Check the registers maintain their value
  EXPECT_EQ(registers.H, byteH);
  EXPECT_EQ(registers.L, byteL);

  // We expect the bytes to be 'logically' reversed due to the little endian
  // ordering
  EXPECT_EQ(registers.HL, wordOne);
}

/**
 * Check that rotating a bit between the 8 bit registers results in the expected
 * 16 bit value
 */
TEST(ProcessorTest, CheckBitRotation) {
  Z80Registers registers;
  byte testByte = 0b1 << 7;
  byte zeroByte = 0;
  word testWord = 0b0000000010000000;

  // Set the 8 bit registers
  registers.H = testByte;
  registers.L = zeroByte;

  // We expect the bytes to be 'logically' reversed due to the little endian
  // ordering
  EXPECT_EQ(registers.HL, testWord);

  // Shift the 16 but register 1 bit
  registers.HL <<= 1;
  EXPECT_EQ(registers.H, 0x0);
  EXPECT_EQ(registers.L, 0x1);
}

/**
 * Check that rotating a bit between the 8 bit registers results in the expected
 * 16 bit value
 */
TEST(ProcessorTest, FlagSettingMacros) {
  Z80Registers registers;
  registers.F = 0x0;

  // Check the flag isn't set
  EXPECT_EQ(GET_FLAG(C_FLAG, registers), 0);

  // Check that we can set the flag
  SET_FLAG(C_FLAG, registers);
  EXPECT_EQ(GET_FLAG(C_FLAG, registers), 1);

  // CHeck that we can reset the flag
  CLEAR_FLAG(C_FLAG, registers);
  EXPECT_EQ(GET_FLAG(C_FLAG, registers), 0);

  // CHeck that no other bits have been set accidentally
  EXPECT_EQ(registers.F, 0);

  // Set all the flags and check that clearing one doesn't affect others
  registers.F = 0xff;
  CLEAR_FLAG(H_FLAG, registers);

  EXPECT_EQ(registers.F, 0b11101111);
}
