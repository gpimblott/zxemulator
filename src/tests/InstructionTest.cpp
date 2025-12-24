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
