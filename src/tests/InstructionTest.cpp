#include "../spectrum/Processor.h"
#include <gtest/gtest.h>
#include <vector>

class InstructionTest : public ::testing::Test {
protected:
  Processor processor;
  ProcessorState *state;

  void SetUp() override {
    // Reset processor state
    processor.reset();
    state = &processor.getState();

    // Ensure memory is clear
    // (Assuming reset does this or we write what we need)
  }

  void executeInstruction(const std::vector<byte> &opcode,
                          word startAddress = 0x8000) {
    // Write opcode to memory
    state->registers.PC = startAddress;
    for (size_t i = 0; i < opcode.size(); ++i) {
      processor.writeMem(startAddress + i, opcode[i]);
    }

    // Execute one instruction
    processor.pause(); // Ensure we can single-step
    processor.step();
    processor.executeFrame();
  }

  void writeBytes(word address, byte value) {
    processor.writeMem(address, value);
  }

  // Helper to check flags
  bool checkFlag(int flag) { return (state->registers.F & flag) != 0; }
};

// Test JP nn (0xC3)
TEST_F(InstructionTest, JP_nn) {
  // JP 0x8010
  // Opcode: C3 10 80
  executeInstruction({0xC3, 0x10, 0x80}, 0x8000);

  EXPECT_EQ(state->registers.PC, 0x8010);
}

// Test CALL nn (0xCD)
TEST_F(InstructionTest, CALL_nn) {
  state->registers.SP = 0xFFFF;

  // CALL 0x8020
  // Opcode: CD 20 80 at 0x8000
  // Expected return address: 0x8003
  executeInstruction({0xCD, 0x20, 0x80}, 0x8000);

  EXPECT_EQ(state->registers.PC, 0x8020);
  EXPECT_EQ(state->registers.SP, 0xFFFD);

  // Check pushed return address
  byte low = state->memory[0xFFFD];
  byte high = state->memory[0xFFFE];
  word returnAddr = (high << 8) | low;
  EXPECT_EQ(returnAddr, 0x8003);
}

// Test RET (0xC9)
TEST_F(InstructionTest, RET) {
  state->registers.SP = 0xFFFD;

  // Push return address 0x9000
  state->memory[0xFFFD] = 0x00;
  state->memory[0xFFFE] = 0x90;

  // RET
  executeInstruction({0xC9}, 0x8000);

  EXPECT_EQ(state->registers.PC, 0x9000);
  EXPECT_EQ(state->registers.SP, 0xFFFF);
}

// Regression Test: Conditional CALL (CALL NZ)
// This verifies the fix where return address must be next instruction (PC+3)
// not current PC
TEST_F(InstructionTest, CALL_NZ_Taken) {
  state->registers.SP = 0xFFFF;

  // Clear Z flag to ensure taken
  state->registers.F &= ~Z_FLAG;

  // CALL NZ, 0x8020 at 0x8000
  // Opcode: C4 20 80
  executeInstruction({0xC4, 0x20, 0x80}, 0x8000);

  EXPECT_EQ(state->registers.PC, 0x8020);

  // Check return address: It should be 0x8003 (instruction size is 3)
  byte low = state->memory[state->registers.SP];
  byte high = state->memory[state->registers.SP + 1];
  word returnAddr = (high << 8) | low;

  EXPECT_EQ(returnAddr, 0x8003);
}

TEST_F(InstructionTest, CALL_NZ_NotTaken) {
  state->registers.SP = 0xFFFF;

  // Set Z flag to ensure NOT taken
  state->registers.F |= Z_FLAG;

  // CALL NZ, 0x8020 at 0x8000
  // Opcode: C4 20 80
  executeInstruction({0xC4, 0x20, 0x80}, 0x8000);

  EXPECT_EQ(state->registers.PC, 0x8003); // Next instruction
  EXPECT_EQ(state->registers.SP, 0xFFFF); // No push
}

// Regression Test: JR e (0x18)
TEST_F(InstructionTest, JR_e) {
  // JR +5
  // Opcode: 18 05 at 0x8000
  // Jump base is 0x8002. +5 = 0x8007.
  executeInstruction({0x18, 0x05}, 0x8000);

  EXPECT_EQ(state->registers.PC, 0x8007);
}

// Regression Test: DJNZ (0x10)
TEST_F(InstructionTest, DJNZ) {
  state->registers.B = 2;

  // DJNZ -2 (FD)
  // Opcode: 10 FD at 0x8000
  // Jump base 0x8002. -3 = 0x7FFF ? No, -3 is FD
  // FD is -3 : 256-3 = 253.
  // Jump base 0x8002. -3 => 0x7FFF.

  // Let's use small positive jump.
  // DJNZ +4
  executeInstruction({0x10, 0x04}, 0x8000);

  EXPECT_EQ(state->registers.B, 1);       // Decremented
  EXPECT_EQ(state->registers.PC, 0x8006); // Took jump (8002+4)

  // Execute again (B becomes 0, no jump)
  // Need to reset PC or loop?
  // Let's test non-taken case separately or re-setup.
  executeInstruction({0x10, 0x04}, 0x8000);
  EXPECT_EQ(state->registers.B, 0);
  EXPECT_EQ(state->registers.PC, 0x8002); // Not taken (Next instruction)
}

// Rotate Opcodes Generation Tests
TEST_F(InstructionTest, RLCA) {
  // 0x07: Rotate Left Circular Accumulator
  // A = 0x80 (10000000). C = 0.
  // Result: A = 0x01 (00000001). C = 1.

  state->registers.A = 0x80;
  state->registers.F = 0; // Clear flags

  executeInstruction({0x07}, 0x8000);

  EXPECT_EQ(state->registers.A, 0x01);
  EXPECT_TRUE(checkFlag(C_FLAG));
  EXPECT_FALSE(checkFlag(H_FLAG));
  EXPECT_FALSE(checkFlag(N_FLAG));
}

TEST_F(InstructionTest, RRCA) {
  // 0x0F: Rotate Right Circular Accumulator
  // A = 0x01 (00000001). C = 0.
  // Result: A = 0x80 (10000000). C = 1.

  state->registers.A = 0x01;
  state->registers.F = 0;

  executeInstruction({0x0F}, 0x8000);

  EXPECT_EQ(state->registers.A, 0x80);
  EXPECT_TRUE(checkFlag(C_FLAG));
  EXPECT_FALSE(checkFlag(H_FLAG));
  EXPECT_FALSE(checkFlag(N_FLAG));
}

TEST_F(InstructionTest, RLA) {
  // 0x17: Rotate Left Accumulator (through Carry)
  // A = 0x80 (10000000). C = 1.
  // Result: A = 0x01 (00000001). C = 1 (from old A7).
  // Wait: A = (A << 1) | OldCarry
  // 0x80 << 1 = 0x00. | 1 = 0x01.
  // New Carry = Old A7 = 1.

  state->registers.A = 0x80;
  state->registers.F = C_FLAG; // Set C

  executeInstruction({0x17}, 0x8000);

  EXPECT_EQ(state->registers.A, 0x01);
  EXPECT_TRUE(checkFlag(C_FLAG));
  EXPECT_FALSE(checkFlag(H_FLAG));
  EXPECT_FALSE(checkFlag(N_FLAG));

  // Test with C=0
  // A = 0x40 (01000000). C = 0.
  // Result: A = 0x80. C = 0.
  state->registers.A = 0x40;
  state->registers.F = 0;
  executeInstruction({0x17}, 0x8000);
  EXPECT_EQ(state->registers.A, 0x80);
  EXPECT_FALSE(checkFlag(C_FLAG));
}

TEST_F(InstructionTest, RRA) {
  // 0x1F: Rotate Right Accumulator (through Carry)
  // A = 0x01 (00000001). C = 1.
  // Result: A = 0x80 (10000000). C = 1 (from old A0).
  // A = (A >> 1) | (OldCarry << 7)
  // 0x01 >> 1 = 0. | (1 << 7) = 0x80.

  state->registers.A = 0x01;
  state->registers.F = C_FLAG;

  executeInstruction({0x1F}, 0x8000);

  EXPECT_EQ(state->registers.A, 0x80);
  EXPECT_TRUE(checkFlag(C_FLAG));
  EXPECT_FALSE(checkFlag(N_FLAG));
}

// IO Opcodes Tests
TEST_F(InstructionTest, OUT_n_A) {
  // 0xD3: OUT (n), A
  // Port 0xFE (254) controls border.
  // We can't easily check side effects (Audio/Video) without mocks,
  // but we can check PC advancement and cycle count if we tracked it.

  state->registers.A = 0x07; // Border Color White (7)

  // OUT (0xFE), A
  // Opcode: D3 FE at 0x8000
  executeInstruction({0xD3, 0xFE}, 0x8000);

  EXPECT_EQ(state->registers.PC, 0x8002);
}

TEST_F(InstructionTest, IN_A_n) {
  // 0xDB: IN A, (n)
  // Read from Port 0xFE (Keyboard)
  // High byte of address (A) selects keyboard row.

  state->registers.A = 0x00;

  // IN A, (0xFE)
  // Opcode: DB FE at 0x8000
  executeInstruction({0xDB, 0xFE}, 0x8000);

  EXPECT_EQ(state->registers.PC, 0x8002);
}

// Bit Opcodes (CB Prefix) Tests
TEST_F(InstructionTest, CB_RLC_B) {
  // 0xCB 0x00: RLC B
  // B = 0x80 (10000000). C = 0.
  // Result: B = 0x01 (00000001). C = 1.

  state->registers.B = 0x80;
  state->registers.F = 0;

  executeInstruction({0xCB, 0x00}, 0x8000);

  EXPECT_EQ(state->registers.B, 0x01);
  EXPECT_TRUE(checkFlag(C_FLAG));
}

TEST_F(InstructionTest, CB_BIT_0_B) {
  // 0xCB 0x40: BIT 0, B
  // B = 0x01. Bit 0 is 1. Z should be 0 (false).

  state->registers.B = 0x01;
  state->registers.F = 0;

  executeInstruction({0xCB, 0x40}, 0x8000);

  EXPECT_FALSE(checkFlag(Z_FLAG));
  EXPECT_TRUE(checkFlag(H_FLAG)); // BIT sets H

  // Test with Bit 0 = 0
  state->registers.B = 0xFE;
  executeInstruction({0xCB, 0x40}, 0x8000);
  EXPECT_TRUE(checkFlag(Z_FLAG));
}

TEST_F(InstructionTest, CB_SET_0_B) {
  // 0xCB 0xC0: SET 0, B
  // B = 0xFE (11111110).
  // Result: B = 0xFF.

  state->registers.B = 0xFE;

  executeInstruction({0xCB, 0xC0}, 0x8000);

  EXPECT_EQ(state->registers.B, 0xFF);
}

TEST_F(InstructionTest, CB_RES_0_B) {
  // 0xCB 0x80: RES 0, B
  // B = 0x01.
  // Result: B = 0x00.

  state->registers.B = 0x01;

  executeInstruction({0xCB, 0x80}, 0x8000);

  EXPECT_EQ(state->registers.B, 0x00);
}

// Extended Opcodes (ED Prefix) Tests
TEST_F(InstructionTest, ED_LDIR_SingleStep) {
  // 0xED 0xB0: LDIR
  // HL -> DE, BC--, HL++, DE++. If BC!=0, PC-=2.

  word src = 0x9000;
  word dest = 0x9100;

  state->registers.HL = src;
  state->registers.DE = dest;
  state->registers.BC = 2; // Copy 2 bytes

  // Write source data
  writeBytes(src, 0xAA);
  writeBytes(src + 1, 0xBB);

  // Execute ONE instruction step (one byte copy)
  executeInstruction({0xED, 0xB0}, 0x8000);

  // Verify first byte copied
  EXPECT_EQ(state->memory[dest], 0xAA);
  // Verify pointers updated
  EXPECT_EQ(state->registers.HL, src + 1);
  EXPECT_EQ(state->registers.DE, dest + 1);
  EXPECT_EQ(state->registers.BC, 1);

  // Verify Loop: PC should be reset to 0x8000 (instruction start) because BC!=0
  EXPECT_EQ(state->registers.PC, 0x8000);

  // Execute Second Step
  // To execute again, we just run executeFrame() or step() again?
  // executeInstruction writes memory and resets PC.
  // We should manually continue from where we are.

  // Let's just run one more step manually
  processor.step();         // Execute next (which is same LDIR)
  processor.executeFrame(); // ACTUALLY execute it

  EXPECT_EQ(state->memory[dest + 1], 0xBB);
  EXPECT_EQ(state->registers.HL, src + 2);
  EXPECT_EQ(state->registers.DE, dest + 2);
  EXPECT_EQ(state->registers.BC, 0);

  // Verify End Of Loop: PC should advance to next instruction (0x8002)
  EXPECT_EQ(state->registers.PC, 0x8002);
}

TEST_F(InstructionTest, ED_LDDR_SingleStep) {
  // 0xED 0xB8: LDDR
  // HL -> DE, BC--, HL--, DE--. If BC!=0, PC-=2.

  word src = 0x9001;
  word dest = 0x9101;

  state->registers.HL = src;
  state->registers.DE = dest;
  state->registers.BC = 1;

  writeBytes(src, 0xCC);

  executeInstruction({0xED, 0xB8}, 0x8000);

  EXPECT_EQ(state->memory[dest], 0xCC);
  EXPECT_EQ(state->registers.HL, src - 1);
  EXPECT_EQ(state->registers.DE, dest - 1);
  EXPECT_EQ(state->registers.BC, 0);

  // BC is 0, so should NOT loop. PC -> 0x8002
  EXPECT_EQ(state->registers.PC, 0x8002);
}
