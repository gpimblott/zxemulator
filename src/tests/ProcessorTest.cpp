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
  word wordOne = 0xE55A; // 58714 - Correct value for H=E5, L=5A

  // Set the 8 bit registers
  registers.H = byteH;
  registers.L = byteL;

  // Check the registers maintain their value
  EXPECT_EQ(registers.H, byteH);
  EXPECT_EQ(registers.L, byteL);

  // We expect HL to be H << 8 | L
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
  word testWord = 0x8000; // H=0x80, L=0x00

  // Set the 8 bit registers
  registers.H = testByte;
  registers.L = zeroByte;

  EXPECT_EQ(registers.HL, testWord);

  // Shift the 16 but register 1 bit
  // 0x8000 << 1 = 0x0000 (overflows 16-bit)
  // Wait, 0x8000 is 1000...0000.
  // Left shift 1 bit in 16-bit arithmetic -> 0.
  // Let's see what the original test expected.
  // Original: testWord = 0x0080. << 1 = 0x0100.
  // registers.H would be 1. registers.L 0.
  // With 0x8000 (32768), << 1 is 65536 -> 0 in 16-bit.
  // Maybe I should change input to be consistent with rotation logic intended.
  // If intended to test shifting 1 bit from L to H:
  // Set L = 0x80 (10000000). H = 0.
  // HL = 0x0080.
  // HL << 1 = 0x0100.
  // H = 1, L = 0.

  // Updating test to utilize L to H shift
  registers.H = 0;
  registers.L = testByte; // 0x80

  EXPECT_EQ(registers.HL, 0x0080);

  registers.HL <<= 1;
  EXPECT_EQ(registers.HL, 0x0100);
  EXPECT_EQ(registers.H, 0x1);
  EXPECT_EQ(registers.L, 0x0);
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
