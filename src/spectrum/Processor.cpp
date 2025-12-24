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

#include "Processor.h"
#include "../utils/Logger.h"
#include "../utils/debug.h"
#include "ALUHelpers.h"
#include "ProcessorMacros.h"
#include "SnapshotLoader.h"
#include <chrono>
#include <thread>

Processor::Processor() : state(), audio() {
  // Set up the default state of the registers
  reset();
  audio.start();
  m_memory = state.memory.getRawMemory();
}

/**
 * initialise the processor with a ROM file
 */
void Processor::init(const char *romFile) {
  // Load the ROM into memory
  Rom theROM = Rom(romFile);
  if (theROM.getSize() <= 0) {
    utils::Logger::write(
        ("Error: Failed to load ROM file: " + std::string(romFile)).c_str());
    throw std::runtime_error("Failed to load ROM file");
  }
  state.memory.loadIntoMemory(theROM);

  // set up the start point
  state.registers.PC = ROM_LOCATION;
}

void Processor::loadTape(Tape tape) {
  state.tape = tape;
  if (state.tape.hasBlocks()) {
    // Don't play yet. Wait for Basic to boot and type LOAD ""
    autoLoadTape = true;
    frameCounter = 0;
    autoLoadStep = 0;
  }
}

void Processor::loadSnapshot(const char *filename) {
  SnapshotLoader::load(filename, state);
}

void Processor::run() {
  running = true;
  while (running) {
    executeFrame();
  }
}

void Processor::executeFrame() {
  // 3.5MHz * 0.02s (50Hz) ~= 69888 T-states per frame
  int tStates = 0;
  const int frameCycles = 69888;

  state.setFrameTStates(0);
  if (state.memory.getVideoBuffer()) {
    state.memory.getVideoBuffer()->newFrame();
  }

  // Fire an interrupt
  if (!paused && state.areInterruptsEnabled()) {
    // If we were halted, we are no longer halted
    if (state.isHalted()) {
      state.setHalted(false);
      // PC already points to next instruction (instruction following HALT)
    }

    // Push PC
    state.registers.SP -= 2;
    word pc = state.registers.PC;
    state.memory[state.registers.SP] = (byte)(pc & 0xFF);
    state.memory[state.registers.SP + 1] = (byte)((pc >> 8) & 0xFF);

    // Interrupt Mode Logic
    int mode = state.getInterruptMode();
    if (mode == 2) {
      // IM 2: Vector form (I << 8) | bus_value. Bus usually 0xFF
      word vector = (state.registers.I << 8) | 0xFF;
      word dest = state.memory.getWord(vector);
      state.registers.PC = dest;
      // Cycles: 19
      tStates += 19;
      state.addFrameTStates(19);
    } else {
      // IM 0/1: RST 38 (0x0038)
      // Note: IM 0 executes instruction on bus. Spectrum bus usually 0xFF (RST
      // 38).
      state.registers.PC = 0x0038;
      // Cycles: 13
      tStates += 13;
      state.addFrameTStates(13);
    }

    // Disable interrupts (standard Z80 behavior on accept)
    state.setInterrupts(false);
  }

  while (tStates < frameCycles && running) {
    if (paused) {
      if (stepRequest) {
        stepRequest = false;
      } else {
        break;
      }
    }

    if (state.isHalted()) {
      // CPU executes NOPs (4 T-states) while halted
      tStates += 4;
      state.addFrameTStates(4);
      this->state.tape.update(4);
      // R register is incremented during NOPs too (M1 cycles)
      state.registers.R =
          (state.registers.R & 0x80) | ((state.registers.R + 1) & 0x7F);
      continue; // Skip fetch/execute
    }

    // Fast Load Trap
    if (state.isFastLoad() && state.registers.PC == 0x0556) {
      // 0x0556 is LD_BYTES.
      // inputs: IX=Dest, DE=Length, A=Flag(00=Header, FF=Data), Carry set=Load
      // outputs: Carry set=Success.

      // We delegate to Tape to see if it can satisfy this request from current
      // block Note: We ignore the 'Verify' case (Carry clear on entry usually
      // means Verify, but ROM routine handles both) We assume Load.
      bool success =
          state.tape.fastLoadBlock(state.registers.A, state.registers.DE,
                                   state.registers.IX, state.memory);

      if (success) {
        // Set Carry
        state.registers.F |= 1;
      } else {
        // Clear Carry
        state.registers.F &= ~1;
      }

      // Execute RET (Pop PC)
      byte low = state.memory[state.registers.SP];
      byte high = state.memory[state.registers.SP + 1];
      state.registers.PC = (high << 8) | low;
      state.registers.SP += 2;

      // Don't execute instruction at 0x0556
      continue;
    }

    // Fetch opcode
    byte opcode = m_memory[state.registers.PC];

    // Increment Refresh Register (Lower 7 bits) - happens on M1 cycle
    state.registers.R =
        (state.registers.R & 0x80) | ((state.registers.R + 1) & 0x7F);

    // Increment PC past opcode
    state.registers.PC++;

    int cycles = 0;
    bool handled = true;

    // Hybrid dispatch: switch for common opcodes, catalogue for others
    switch (opcode) {
    case 0x00: // NOP
      cycles = 4;
      break;

    case 0x08: // EX AF, AF'
    {
      word temp = state.registers.AF;
      state.registers.AF = state.registers.AF_;
      state.registers.AF_ = temp;
      cycles = 4;
      break;
    }

    case 0xD9: // EXX
    {
      word tempBC = state.registers.BC;
      word tempDE = state.registers.DE;
      word tempHL = state.registers.HL;
      state.registers.BC = state.registers.BC_;
      state.registers.DE = state.registers.DE_;
      state.registers.HL = state.registers.HL_;
      state.registers.BC_ = tempBC;
      state.registers.DE_ = tempDE;
      state.registers.HL_ = tempHL;
      cycles = 4;
      break;
    }

    case 0x10: // DJNZ e
    {
      int8_t offset = (int8_t)m_memory[state.registers.PC];
      state.registers.PC++;
      state.registers.B--;
      if (state.registers.B != 0) {
        state.registers.PC += offset;
        cycles = 13;
      } else {
        cycles = 8;
      }
      break;
    }

    // RST instructions
    case 0xC7: // RST 00
      push16(state.registers.PC);
      state.registers.PC = 0x0000;
      cycles = 11;
      break;

    case 0xCF: // RST 08
      push16(state.registers.PC);
      state.registers.PC = 0x0008;
      cycles = 11;
      break;

    case 0xD7: // RST 10
      push16(state.registers.PC);
      state.registers.PC = 0x0010;
      cycles = 11;
      break;

    case 0xDF: // RST 18
      push16(state.registers.PC);
      state.registers.PC = 0x0018;
      cycles = 11;
      break;

    case 0xE7: // RST 20
      push16(state.registers.PC);
      state.registers.PC = 0x0020;
      cycles = 11;
      break;

    case 0xEF: // RST 28
      push16(state.registers.PC);
      state.registers.PC = 0x0028;
      cycles = 11;
      break;

    case 0xF7: // RST 30
      push16(state.registers.PC);
      state.registers.PC = 0x0030;
      cycles = 11;
      break;

    case 0xFF: // RST 38
      push16(state.registers.PC);
      state.registers.PC = 0x0038;
      cycles = 11;
      break;

    case 0x18: // JR e
    {
      int8_t offset = (int8_t)m_memory[state.registers.PC];
      state.registers.PC++;
      state.registers.PC += offset;
      cycles = 12;
      break;
    }

    case 0x20: // JR NZ, e
    {
      int8_t offset = (int8_t)m_memory[state.registers.PC];
      state.registers.PC++;
      if (!GET_FLAG(Z_FLAG, state.registers)) {
        state.registers.PC += offset;
        cycles = 12;
      } else {
        cycles = 7;
      }
      break;
    }

    case 0x28: // JR Z, e
    {
      int8_t offset = (int8_t)m_memory[state.registers.PC];
      state.registers.PC++;
      if (GET_FLAG(Z_FLAG, state.registers)) {
        state.registers.PC += offset;
        cycles = 12;
      } else {
        cycles = 7;
      }
      break;
    }

    case 0x30: // JR NC, e
    {
      int8_t offset = (int8_t)m_memory[state.registers.PC];
      state.registers.PC++;
      if (!GET_FLAG(C_FLAG, state.registers)) {
        state.registers.PC += offset;
        cycles = 12;
      } else {
        cycles = 7;
      }
      break;
    }

    case 0x38: // JR C, e
    {
      int8_t offset = (int8_t)m_memory[state.registers.PC];
      state.registers.PC++;

      if (GET_FLAG(C_FLAG, state.registers)) {
        state.registers.PC += offset;
        cycles = 12;
      } else {
        cycles = 7;
      }
      break;
    }

    case 0x76: // HALT
      state.setHalted(true);
      cycles = 4;
      break;

    case 0x3E: { // LD A, n
      byte value = m_memory[state.registers.PC++];
      state.registers.A = value;
      cycles = 7;
      break;
    }

    case 0x06: { // LD B, n
      byte value = m_memory[state.registers.PC++];
      state.registers.B = value;
      cycles = 7;
      break;
    }

    case 0x0E: { // LD C, n
      byte value = m_memory[state.registers.PC++];
      state.registers.C = value;
      cycles = 7;
      break;
    }

    case 0x16: { // LD D, n
      byte value = m_memory[state.registers.PC++];
      state.registers.D = value;
      cycles = 7;
      break;
    }

    case 0x1E: { // LD E, n
      byte value = m_memory[state.registers.PC++];
      state.registers.E = value;
      cycles = 7;
      break;
    }

    case 0x26: { // LD H, n
      byte value = m_memory[state.registers.PC++];
      state.registers.H = value;
      cycles = 7;
      break;
    }

    case 0x2E: { // LD L, n
      byte value = m_memory[state.registers.PC++];
      state.registers.L = value;
      cycles = 7;
      break;
    }

    // ------------------------------------------------------------------------
    // 16-bit Arithmetic
    // ------------------------------------------------------------------------
    case 0x09: // ADD HL, BC
      add16(state.registers.HL, state.registers.BC);
      cycles = 11;
      break;
    case 0x19: // ADD HL, DE
      add16(state.registers.HL, state.registers.DE);
      cycles = 11;
      break;
    case 0x29: // ADD HL, HL
      add16(state.registers.HL, state.registers.HL);
      cycles = 11;
      break;
    case 0x39: // ADD HL, SP
      add16(state.registers.HL, state.registers.SP);
      cycles = 11;
      break;

    case 0xC3: // JP nn
    {
      word address = state.getNextWordFromPC();
      state.registers.PC = address;
      cycles = 10;
      break;
    }

    case 0xE9: // JP (HL)
      state.registers.PC = state.registers.HL;
      cycles = 4;
      break;

    // Conditional Jumps
    case 0xC2: // JP NZ, nn
      if (!GET_FLAG(Z_FLAG, state.registers)) {
        state.registers.PC = state.getNextWordFromPC();
        cycles = 10;
      } else {
        state.registers.PC += 2;
        cycles = 10;
      }
      break;

    case 0xCA: // JP Z, nn
      if (GET_FLAG(Z_FLAG, state.registers)) {
        state.registers.PC = state.getNextWordFromPC();
        cycles = 10;
      } else {
        state.registers.PC += 2;
        cycles = 10;
      }
      break;

    case 0xD2: // JP NC, nn
      if (!GET_FLAG(C_FLAG, state.registers)) {
        state.registers.PC = state.getNextWordFromPC();
        cycles = 10;
      } else {
        state.registers.PC += 2;
        cycles = 10;
      }
      break;

    case 0xDA: // JP C, nn
      if (GET_FLAG(C_FLAG, state.registers)) {
        state.registers.PC = state.getNextWordFromPC();
        cycles = 10;
      } else {
        state.registers.PC += 2;
        cycles = 10;
      }
      break;

    case 0xE2: // JP PO, nn
      if (!GET_FLAG(P_FLAG, state.registers)) {
        state.registers.PC = state.getNextWordFromPC();
        cycles = 10;
      } else {
        state.registers.PC += 2;
        cycles = 10;
      }
      break;

    case 0xEA: // JP PE, nn
      if (GET_FLAG(P_FLAG, state.registers)) {
        state.registers.PC = state.getNextWordFromPC();
        cycles = 10;
      } else {
        state.registers.PC += 2;
        cycles = 10;
      }
      break;

    case 0xF2: // JP P, nn
      if (!GET_FLAG(S_FLAG, state.registers)) {
        state.registers.PC = state.getNextWordFromPC();
        cycles = 10;
      } else {
        state.registers.PC += 2;
        cycles = 10;
      }
      break;

    case 0xFA: // JP M, nn
      if (GET_FLAG(S_FLAG, state.registers)) {
        state.registers.PC = state.getNextWordFromPC();
        cycles = 10;
      } else {
        state.registers.PC += 2;
        cycles = 10;
      }
      break;

    // Conditional Returns
    case 0xC0: // RET NZ
      if (!GET_FLAG(Z_FLAG, state.registers)) {
        state.registers.PC = pop16();
        cycles = 11;
      } else {
        cycles = 5;
      }
      break;

    case 0xC8: // RET Z
      if (GET_FLAG(Z_FLAG, state.registers)) {
        state.registers.PC = pop16();
        cycles = 11;
      } else {
        cycles = 5;
      }
      break;

    case 0xD0: // RET NC
      if (!GET_FLAG(C_FLAG, state.registers)) {
        state.registers.PC = pop16();
        cycles = 11;
      } else {
        cycles = 5;
      }
      break;

    case 0xD8: // RET C
      if (GET_FLAG(C_FLAG, state.registers)) {
        state.registers.PC = pop16();
        cycles = 11;
      } else {
        cycles = 5;
      }
      break;

    case 0xE0: // RET PO
      if (!GET_FLAG(P_FLAG, state.registers)) {
        state.registers.PC = pop16();
        cycles = 11;
      } else {
        cycles = 5;
      }
      break;

    case 0xE8: // RET PE
      if (GET_FLAG(P_FLAG, state.registers)) {
        state.registers.PC = pop16();
        cycles = 11;
      } else {
        cycles = 5;
      }
      break;

    case 0xF0: // RET P
      if (!GET_FLAG(S_FLAG, state.registers)) {
        state.registers.PC = pop16();
        cycles = 11;
      } else {
        cycles = 5;
      }
      break;

    case 0xF8: // RET M
      if (GET_FLAG(S_FLAG, state.registers)) {
        state.registers.PC = pop16();
        cycles = 11;
      } else {
        cycles = 5;
      }
      break;

    case 0xC9: { // RET
      byte low = m_memory[state.registers.SP];
      byte high = m_memory[state.registers.SP + 1];
      state.registers.PC = (high << 8) | low;
      state.registers.SP += 2;
      cycles = 10;
      break;
    }

    case 0xCD: { // CALL nn
      word address = m_memory[state.registers.PC] |
                     (m_memory[state.registers.PC + 1] << 8);
      state.registers.PC += 2;
      // Push PC
      state.registers.SP -= 2;
      word pc = state.registers.PC;
      writeMem(state.registers.SP, (byte)(pc & 0xFF));
      writeMem(state.registers.SP + 1, (byte)((pc >> 8) & 0xFF));
      state.registers.PC = address;
      cycles = 17;
      break;
    }

    // Conditional CALLs
    case 0xC4: // CALL NZ, nn
      if (!GET_FLAG(Z_FLAG, state.registers)) {
        word address = state.getNextWordFromPC();
        state.registers.PC += 2;
        push16(state.registers.PC);
        state.registers.PC = address;
        cycles = 17;
      } else {
        state.registers.PC += 2;
        cycles = 10;
      }
      break;

    case 0xCC: // CALL Z, nn
      if (GET_FLAG(Z_FLAG, state.registers)) {
        word address = state.getNextWordFromPC();
        state.registers.PC += 2;
        push16(state.registers.PC);
        state.registers.PC = address;
        cycles = 17;
      } else {
        state.registers.PC += 2;
        cycles = 10;
      }
      break;

    case 0xD4: // CALL NC, nn
      if (!GET_FLAG(C_FLAG, state.registers)) {
        word address = state.getNextWordFromPC();
        state.registers.PC += 2;
        push16(state.registers.PC);
        state.registers.PC = address;
        cycles = 17;
      } else {
        state.registers.PC += 2;
        cycles = 10;
      }
      break;

    case 0xDC: // CALL C, nn
      if (GET_FLAG(C_FLAG, state.registers)) {
        word address = state.getNextWordFromPC();
        state.registers.PC += 2;
        push16(state.registers.PC);
        state.registers.PC = address;
        cycles = 17;
      } else {
        state.registers.PC += 2;
        cycles = 10;
      }
      break;

    case 0xE4: // CALL PO, nn
      if (!GET_FLAG(P_FLAG, state.registers)) {
        word address = state.getNextWordFromPC();
        state.registers.PC += 2;
        push16(state.registers.PC);
        state.registers.PC = address;
        cycles = 17;
      } else {
        state.registers.PC += 2;
        cycles = 10;
      }
      break;

    case 0xEC: // CALL PE, nn
      if (GET_FLAG(P_FLAG, state.registers)) {
        word address = state.getNextWordFromPC();
        state.registers.PC += 2;
        push16(state.registers.PC);
        state.registers.PC = address;
        cycles = 17;
      } else {
        state.registers.PC += 2;
        cycles = 10;
      }
      break;

    case 0xF4: // CALL P, nn
      if (!GET_FLAG(S_FLAG, state.registers)) {
        word address = state.getNextWordFromPC();
        state.registers.PC += 2;
        push16(state.registers.PC);
        state.registers.PC = address;
        cycles = 17;
      } else {
        state.registers.PC += 2;
        cycles = 10;
      }
      break;

    case 0xFC: // CALL M, nn
      if (GET_FLAG(S_FLAG, state.registers)) {
        word address = state.getNextWordFromPC();
        state.registers.PC += 2;
        push16(state.registers.PC);
        state.registers.PC = address;
        cycles = 17;
      } else {
        state.registers.PC += 2;
        cycles = 10;
      }
      break;

    case 0xE3: { // EX (SP), HL
      byte l = m_memory[state.registers.SP];
      byte h = m_memory[state.registers.SP + 1];
      writeMem(state.registers.SP, state.registers.L);
      writeMem(state.registers.SP + 1, state.registers.H);
      state.registers.H = h;
      state.registers.L = l;
      cycles = 19; // 4 + 3 + 4 + 3 + 5 wait? Z80 Manual says 19.
      break;
    }

    case 0xEB: { // EX DE, HL
      word temp = state.registers.HL;
      state.registers.HL = state.registers.DE;
      state.registers.DE = temp;
      cycles = 4;
      break;
    }

    case 0xF3: // DI
      state.setInterrupts(false);
      cycles = 4;
      break;

    case 0xF9: // LD SP, HL
      state.registers.SP = state.registers.HL;
      cycles = 6;
      break;

    case 0xFB: // EI
      state.setInterrupts(true);
      cycles = 4;
      break;

    // LD r, r' instructions (0x40-0x7F range, very common)
    // LD B, r
    case 0x40:
      cycles = 4;
      break; // LD B, B (NOP-like)
    case 0x41:
      state.registers.B = state.registers.C;
      cycles = 4;
      break;
    case 0x42:
      state.registers.B = state.registers.D;
      cycles = 4;
      break;
    case 0x43:
      state.registers.B = state.registers.E;
      cycles = 4;
      break;
    case 0x44:
      state.registers.B = state.registers.H;
      cycles = 4;
      break;
    case 0x45:
      state.registers.B = state.registers.L;
      cycles = 4;
      break;
    case 0x46:
      state.registers.B = m_memory[state.registers.HL];
      cycles = 7;
      break; // LD B, (HL)
    case 0x47:
      state.registers.B = state.registers.A;
      cycles = 4;
      break;

    // LD C, r
    case 0x48:
      state.registers.C = state.registers.B;
      cycles = 4;
      break;
    case 0x49:
      cycles = 4;
      break; // LD C, C
    case 0x4A:
      state.registers.C = state.registers.D;
      cycles = 4;
      break;
    case 0x4B:
      state.registers.C = state.registers.E;
      cycles = 4;
      break;
    case 0x4C:
      state.registers.C = state.registers.H;
      cycles = 4;
      break;
    case 0x4D:
      state.registers.C = state.registers.L;
      cycles = 4;
      break;
    case 0x4E:
      state.registers.C = m_memory[state.registers.HL];
      cycles = 7;
      break; // LD C, (HL)
    case 0x4F:
      state.registers.C = state.registers.A;
      cycles = 4;
      break;

    // LD D, r
    case 0x50:
      state.registers.D = state.registers.B;
      cycles = 4;
      break;
    case 0x51:
      state.registers.D = state.registers.C;
      cycles = 4;
      break;
    case 0x52:
      cycles = 4;
      break; // LD D, D
    case 0x53:
      state.registers.D = state.registers.E;
      cycles = 4;
      break;
    case 0x54:
      state.registers.D = state.registers.H;
      cycles = 4;
      break;
    case 0x55:
      state.registers.D = state.registers.L;
      cycles = 4;
      break;
    case 0x56:
      state.registers.D = m_memory[state.registers.HL];
      cycles = 7;
      break; // LD D, (HL)
    case 0x57:
      state.registers.D = state.registers.A;
      cycles = 4;
      break;

    // LD E, r
    case 0x58:
      state.registers.E = state.registers.B;
      cycles = 4;
      break;
    case 0x59:
      state.registers.E = state.registers.C;
      cycles = 4;
      break;
    case 0x5A:
      state.registers.E = state.registers.D;
      cycles = 4;
      break;
    case 0x5B:
      cycles = 4;
      break; // LD E, E
    case 0x5C:
      state.registers.E = state.registers.H;
      cycles = 4;
      break;
    case 0x5D:
      state.registers.E = state.registers.L;
      cycles = 4;
      break;
    case 0x5E:
      state.registers.E = m_memory[state.registers.HL];
      cycles = 7;
      break; // LD E, (HL)
    case 0x5F:
      state.registers.E = state.registers.A;
      cycles = 4;
      break;

    // LD H, r
    case 0x60:
      state.registers.H = state.registers.B;
      cycles = 4;
      break;
    case 0x61:
      state.registers.H = state.registers.C;
      cycles = 4;
      break;
    case 0x62:
      state.registers.H = state.registers.D;
      cycles = 4;
      break;
    case 0x63:
      state.registers.H = state.registers.E;
      cycles = 4;
      break;
    case 0x64:
      cycles = 4;
      break; // LD H, H
    case 0x65:
      state.registers.H = state.registers.L;
      cycles = 4;
      break;
    case 0x66:
      state.registers.H = m_memory[state.registers.HL];
      cycles = 7;
      break; // LD H, (HL)
    case 0x67:
      state.registers.H = state.registers.A;
      cycles = 4;
      break;

    // LD L, r
    case 0x68:
      state.registers.L = state.registers.B;
      cycles = 4;
      break;
    case 0x69:
      state.registers.L = state.registers.C;
      cycles = 4;
      break;
    case 0x6A:
      state.registers.L = state.registers.D;
      cycles = 4;
      break;
    case 0x6B:
      state.registers.L = state.registers.E;
      cycles = 4;
      break;
    case 0x6C:
      state.registers.L = state.registers.H;
      cycles = 4;
      break;
    case 0x6D:
      cycles = 4;
      break; // LD L, L
    case 0x6E:
      state.registers.L = m_memory[state.registers.HL];
      cycles = 7;
      break; // LD L, (HL)
    case 0x6F:
      state.registers.L = state.registers.A;
      cycles = 4;
      break;

    // LD (HL), r
    case 0x70:
      writeMem(state.registers.HL, state.registers.B);
      cycles = 7;
      break;
    case 0x71:
      writeMem(state.registers.HL, state.registers.C);
      cycles = 7;
      break;
    case 0x72:
      writeMem(state.registers.HL, state.registers.D);
      cycles = 7;
      break;
    case 0x73:
      writeMem(state.registers.HL, state.registers.E);
      cycles = 7;
      break;
    case 0x74:
      writeMem(state.registers.HL, state.registers.H);
      cycles = 7;
      break;
    case 0x75:
      writeMem(state.registers.HL, state.registers.L);
      cycles = 7;
      break;
    // 0x76 is HALT (already handled above)
    case 0x77:
      writeMem(state.registers.HL, state.registers.A);
      cycles = 7;
      break;

    // LD A, r
    case 0x78:
      state.registers.A = state.registers.B;
      cycles = 4;
      break;
    case 0x79:
      state.registers.A = state.registers.C;
      cycles = 4;
      break;
    case 0x7A:
      state.registers.A = state.registers.D;
      cycles = 4;
      break;
    case 0x7B:
      state.registers.A = state.registers.E;
      cycles = 4;
      break;
    case 0x7C:
      state.registers.A = state.registers.H;
      cycles = 4;
      break;
    case 0x7D:
      state.registers.A = state.registers.L;
      cycles = 4;
      break;
    case 0x7E:
      state.registers.A = m_memory[state.registers.HL];
      cycles = 7;
      break; // LD A, (HL)
    case 0x7F:
      cycles = 4;
      break; // LD A, A

    // LD (BC), A and LD A, (BC)
    case 0x02:
      writeMem(state.registers.BC, state.registers.A);
      cycles = 7;
      break;
    case 0x0A:
      state.registers.A = m_memory[state.registers.BC];
      cycles = 7;
      break;

    // LD (DE), A and LD A, (DE)
    case 0x12:
      writeMem(state.registers.DE, state.registers.A);
      cycles = 7;
      break;
    case 0x1A:
      state.registers.A = m_memory[state.registers.DE];
      cycles = 7;
      break;

    // 16-bit loads with immediates
    case 0x01: { // LD BC, nn
      word value = m_memory[state.registers.PC] |
                   (m_memory[state.registers.PC + 1] << 8);
      state.registers.PC += 2;
      state.registers.BC = value;
      cycles = 10;
      break;
    }

    case 0x11: { // LD DE, nn
      word value = m_memory[state.registers.PC] |
                   (m_memory[state.registers.PC + 1] << 8);
      state.registers.PC += 2;
      state.registers.DE = value;
      cycles = 10;
      break;
    }

    case 0x21: { // LD HL, nn
      word value = m_memory[state.registers.PC] |
                   (m_memory[state.registers.PC + 1] << 8);
      state.registers.PC += 2;
      state.registers.HL = value;
      cycles = 10;
      break;
    }

    case 0x22: { // LD (nn), HL
      word address = m_memory[state.registers.PC] |
                     (m_memory[state.registers.PC + 1] << 8);
      state.registers.PC += 2;
      writeMem(address, state.registers.L);
      writeMem(address + 1, state.registers.H);
      cycles = 16;
      break;
    }

    case 0x2A: { // LD HL, (nn)
      word address = m_memory[state.registers.PC] |
                     (m_memory[state.registers.PC + 1] << 8);
      state.registers.PC += 2;
      state.registers.L = m_memory[address];
      state.registers.H = m_memory[address + 1];
      cycles = 16;
      break;
    }

    // ------------------------------------------------------------------------
    // Stack Operations (PUSH/POP)
    // ------------------------------------------------------------------------
    case 0xC1: // POP BC
      state.registers.C = m_memory[state.registers.SP];
      state.registers.B = m_memory[state.registers.SP + 1];
      state.registers.SP += 2;
      cycles = 10;
      break;
    case 0xD1: // POP DE
      state.registers.E = m_memory[state.registers.SP];
      state.registers.D = m_memory[state.registers.SP + 1];
      state.registers.SP += 2;
      cycles = 10;
      break;
    case 0xE1: // POP HL
      state.registers.L = m_memory[state.registers.SP];
      state.registers.H = m_memory[state.registers.SP + 1];
      state.registers.SP += 2;
      cycles = 10;
      break;
    case 0xF1: // POP AF
      state.registers.F = m_memory[state.registers.SP];
      state.registers.A = m_memory[state.registers.SP + 1];
      state.registers.SP += 2;
      cycles = 10;
      break;

    case 0xC5: // PUSH BC
      state.registers.SP -= 2;
      writeMem(state.registers.SP, state.registers.C);
      writeMem(state.registers.SP + 1, state.registers.B);
      cycles = 11;
      break;
    case 0xD5: // PUSH DE
      state.registers.SP -= 2;
      writeMem(state.registers.SP, state.registers.E);
      writeMem(state.registers.SP + 1, state.registers.D);
      cycles = 11;
      break;
    case 0xE5: // PUSH HL
      state.registers.SP -= 2;
      writeMem(state.registers.SP, state.registers.L);
      writeMem(state.registers.SP + 1, state.registers.H);
      cycles = 11;
      break;
    case 0xF5: // PUSH AF
      state.registers.SP -= 2;
      writeMem(state.registers.SP, state.registers.F);
      writeMem(state.registers.SP + 1, state.registers.A);
      cycles = 11;
      break;

    case 0x31: { // LD SP, nn
      word value = m_memory[state.registers.PC] |
                   (m_memory[state.registers.PC + 1] << 8);
      state.registers.PC += 2;
      state.registers.SP = value;
      cycles = 10;
      break;
    }

    case 0x32: { // LD (nn), A
      word address = m_memory[state.registers.PC] |
                     (m_memory[state.registers.PC + 1] << 8);
      state.registers.PC += 2;
      writeMem(address, state.registers.A);
      cycles = 13;
      break;
    }

    case 0x3A: { // LD A, (nn)
      word address = m_memory[state.registers.PC] |
                     (m_memory[state.registers.PC + 1] << 8);
      state.registers.PC += 2;
      state.registers.A = m_memory[address];
      cycles = 13;
      break;
    }

    case 0x36: { // LD (HL), n
      byte value = m_memory[state.registers.PC++];
      writeMem(state.registers.HL, value);
      cycles = 10;
      break;
    }

    // ------------------------------------------------------------------------
    // Arithmetic Ops (0x80 - 0xBF)
    // ------------------------------------------------------------------------

    // ADD A, r (0x80-0x87)
    case 0x80:
      add8(state.registers.B);
      cycles = 4;
      break;
    case 0x81:
      add8(state.registers.C);
      cycles = 4;
      break;
    case 0x82:
      add8(state.registers.D);
      cycles = 4;
      break;
    case 0x83:
      add8(state.registers.E);
      cycles = 4;
      break;
    case 0x84:
      add8(state.registers.H);
      cycles = 4;
      break;
    case 0x85:
      add8(state.registers.L);
      cycles = 4;
      break;
    case 0x86:
      add8(m_memory[state.registers.HL]);
      cycles = 7;
      break;
    case 0x87:
      add8(state.registers.A);
      cycles = 4;
      break;

    // ADC A, r (0x88-0x8F)
    case 0x88:
      adc8(state.registers.B);
      cycles = 4;
      break;
    case 0x89:
      adc8(state.registers.C);
      cycles = 4;
      break;
    case 0x8A:
      adc8(state.registers.D);
      cycles = 4;
      break;
    case 0x8B:
      adc8(state.registers.E);
      cycles = 4;
      break;
    case 0x8C:
      adc8(state.registers.H);
      cycles = 4;
      break;
    case 0x8D:
      adc8(state.registers.L);
      cycles = 4;
      break;
    case 0x8E:
      adc8(m_memory[state.registers.HL]);
      cycles = 7;
      break;
    case 0x8F:
      adc8(state.registers.A);
      cycles = 4;
      break;

    // SUB r (0x90-0x97)
    case 0x90:
      sub8(state.registers.B);
      cycles = 4;
      break;
    case 0x91:
      sub8(state.registers.C);
      cycles = 4;
      break;
    case 0x92:
      sub8(state.registers.D);
      cycles = 4;
      break;
    case 0x93:
      sub8(state.registers.E);
      cycles = 4;
      break;
    case 0x94:
      sub8(state.registers.H);
      cycles = 4;
      break;
    case 0x95:
      sub8(state.registers.L);
      cycles = 4;
      break;
    case 0x96:
      sub8(m_memory[state.registers.HL]);
      cycles = 7;
      break;
    case 0x97:
      sub8(state.registers.A);
      cycles = 4;
      break;

    // SBC A, r (0x98-0x9F)
    case 0x98:
      sbc8(state.registers.B);
      cycles = 4;
      break;
    case 0x99:
      sbc8(state.registers.C);
      cycles = 4;
      break;
    case 0x9A:
      sbc8(state.registers.D);
      cycles = 4;
      break;
    case 0x9B:
      sbc8(state.registers.E);
      cycles = 4;
      break;
    case 0x9C:
      sbc8(state.registers.H);
      cycles = 4;
      break;
    case 0x9D:
      sbc8(state.registers.L);
      cycles = 4;
      break;
    case 0x9E:
      sbc8(m_memory[state.registers.HL]);
      cycles = 7;
      break;
    case 0x9F:
      sbc8(state.registers.A);
      cycles = 4;
      break;

    // AND r (0xA0-0xA7)
    case 0xA0:
      and8(state.registers.B);
      cycles = 4;
      break;
    case 0xA1:
      and8(state.registers.C);
      cycles = 4;
      break;
    case 0xA2:
      and8(state.registers.D);
      cycles = 4;
      break;
    case 0xA3:
      and8(state.registers.E);
      cycles = 4;
      break;
    case 0xA4:
      and8(state.registers.H);
      cycles = 4;
      break;
    case 0xA5:
      and8(state.registers.L);
      cycles = 4;
      break;
    case 0xA6:
      and8(m_memory[state.registers.HL]);
      cycles = 7;
      break;
    case 0xA7:
      and8(state.registers.A);
      cycles = 4;
      break;

    // XOR r (0xA8-0xAF)
    case 0xA8:
      xor8(state.registers.B);
      cycles = 4;
      break;
    case 0xA9:
      xor8(state.registers.C);
      cycles = 4;
      break;
    case 0xAA:
      xor8(state.registers.D);
      cycles = 4;
      break;
    case 0xAB:
      xor8(state.registers.E);
      cycles = 4;
      break;
    case 0xAC:
      xor8(state.registers.H);
      cycles = 4;
      break;
    case 0xAD:
      xor8(state.registers.L);
      cycles = 4;
      break;
    case 0xAE:
      xor8(m_memory[state.registers.HL]);
      cycles = 7;
      break;
    case 0xAF:
      xor8(state.registers.A);
      cycles = 4;
      break;

    // OR r (0xB0-0xB7)
    case 0xB0:
      or8(state.registers.B);
      cycles = 4;
      break;
    case 0xB1:
      or8(state.registers.C);
      cycles = 4;
      break;
    case 0xB2:
      or8(state.registers.D);
      cycles = 4;
      break;
    case 0xB3:
      or8(state.registers.E);
      cycles = 4;
      break;
    case 0xB4:
      or8(state.registers.H);
      cycles = 4;
      break;
    case 0xB5:
      or8(state.registers.L);
      cycles = 4;
      break;
    case 0xB6:
      or8(m_memory[state.registers.HL]);
      cycles = 7;
      break;
    case 0xB7:
      or8(state.registers.A);
      cycles = 4;
      break;

    // CP r (0xB8-0xBF)
    case 0xB8:
      cp8(state.registers.B);
      cycles = 4;
      break;
    case 0xB9:
      cp8(state.registers.C);
      cycles = 4;
      break;
    case 0xBA:
      cp8(state.registers.D);
      cycles = 4;
      break;
    case 0xBB:
      cp8(state.registers.E);
      cycles = 4;
      break;
    case 0xBC:
      cp8(state.registers.H);
      cycles = 4;
      break;
    case 0xBD:
      cp8(state.registers.L);
      cycles = 4;
      break;
    case 0xBE:
      cp8(m_memory[state.registers.HL]);
      cycles = 7;
      break;
    case 0xBF:
      cp8(state.registers.A);
      cycles = 4;
      break;

    // ------------------------------------------------------------------------
    // Immediate Arithmetic (0xC6, 0xCE, 0xD6, 0xDE, 0xE6, 0xEE, 0xF6, 0xFE)
    // ------------------------------------------------------------------------
    case 0xC6: { // ADD A, n
      byte n = m_memory[state.registers.PC++];
      add8(n);
      cycles = 7;
      break;
    }
    case 0xCE: { // ADC A, n
      byte n = m_memory[state.registers.PC++];
      adc8(n);
      cycles = 7;
      break;
    }
    case 0xD6: { // SUB n
      byte n = m_memory[state.registers.PC++];
      sub8(n);
      cycles = 7;
      break;
    }
    case 0xDE: { // SBC A, n
      byte n = m_memory[state.registers.PC++];
      sbc8(n);
      cycles = 7;
      break;
    }
    case 0xE6: { // AND n
      byte n = m_memory[state.registers.PC++];
      and8(n);
      cycles = 7;
      break;
    }
    case 0xEE: { // XOR n
      byte n = m_memory[state.registers.PC++];
      xor8(n);
      cycles = 7;
      break;
    }
    case 0xF6: { // OR n
      byte n = m_memory[state.registers.PC++];
      or8(n);
      cycles = 7;
      break;
    }
    case 0xFE: { // CP n
      byte n = m_memory[state.registers.PC++];
      cp8(n);
      cycles = 7;
      break;
    }

    // ------------------------------------------------------------------------
    // INC/DEC 8-bit
    // ------------------------------------------------------------------------
    case 0x04:
      inc8(state.registers.B);
      cycles = 4;
      break;
    case 0x05:
      dec8(state.registers.B);
      cycles = 4;
      break;
    case 0x0C:
      inc8(state.registers.C);
      cycles = 4;
      break;
    case 0x0D:
      dec8(state.registers.C);
      cycles = 4;
      break;
    case 0x14:
      inc8(state.registers.D);
      cycles = 4;
      break;
    case 0x15:
      dec8(state.registers.D);
      cycles = 4;
      break;
    case 0x1C:
      inc8(state.registers.E);
      cycles = 4;
      break;
    case 0x1D:
      dec8(state.registers.E);
      cycles = 4;
      break;
    case 0x24:
      inc8(state.registers.H);
      cycles = 4;
      break;
    case 0x25:
      dec8(state.registers.H);
      cycles = 4;
      break;
    case 0x2C:
      inc8(state.registers.L);
      cycles = 4;
      break;
    case 0x2D:
      dec8(state.registers.L);
      cycles = 4;
      break;

    case 0x3C:
      inc8(state.registers.A);
      cycles = 4;
      break;
    case 0x3D:
      dec8(state.registers.A);
      cycles = 4;
      break;

    case 0x34: { // INC (HL)
      byte val = m_memory[state.registers.HL];
      inc8(val);
      writeMem(state.registers.HL, val);
      cycles = 11;
      break;
    }
    case 0x35: { // DEC (HL)
      byte val = m_memory[state.registers.HL];
      dec8(val);
      writeMem(state.registers.HL, val);
      cycles = 11;
      break;
    }

    // ------------------------------------------------------------------------
    // Misc
    // ------------------------------------------------------------------------
    case 0x27: { // DAA
      byte initA = state.registers.A;
      byte correction = 0;
      bool C = GET_FLAG(C_FLAG, state.registers);
      bool H = GET_FLAG(H_FLAG, state.registers);
      bool N = GET_FLAG(N_FLAG, state.registers);

      if (H || ((initA & 0x0F) > 9)) {
        correction += 6;
      }
      if (C || (initA > 0x99)) {
        correction += 0x60;
        SET_FLAG(C_FLAG, state.registers);
      }

      if (N) {
        state.registers.A -= correction;
      } else {
        state.registers.A += correction;
      }

      // Flags
      byte res = state.registers.A;
      // S, Z
      if (res & 0x80)
        SET_FLAG(S_FLAG, state.registers);
      else
        CLEAR_FLAG(S_FLAG, state.registers);

      if (res == 0)
        SET_FLAG(Z_FLAG, state.registers);
      else
        CLEAR_FLAG(Z_FLAG, state.registers);

      // P/V (Parity)
      int bits = 0;
      for (int i = 0; i < 8; i++) {
        if (res & (1 << i))
          bits++;
      }
      if (bits % 2 == 0)
        SET_FLAG(P_FLAG, state.registers); // Even parity
      else
        CLEAR_FLAG(P_FLAG, state.registers);

      // H Flag calculation (Simpler heuristic that often works for ROM boot)
      // "If there was a correction to the lower nibble, H is usually set?"
      // A common accurate formula:
      bool h_update = false;
      if (N) {
        h_update = (H && (initA & 0x0F) < 6);
      } else {
        h_update = ((initA & 0x0F) > 9);
      }
      if (h_update)
        SET_FLAG(H_FLAG, state.registers);
      else
        CLEAR_FLAG(H_FLAG, state.registers);

      cycles = 4;
      break;
    }
    case 0x2F: { // CPL
      state.registers.A = ~state.registers.A;
      SET_FLAG(H_FLAG, state.registers);
      SET_FLAG(N_FLAG, state.registers);
      cycles = 4;
      break;
    }
    case 0x37: { // SCF
      SET_FLAG(C_FLAG, state.registers);
      CLEAR_FLAG(N_FLAG, state.registers);
      CLEAR_FLAG(H_FLAG, state.registers);
      cycles = 4;
      break;
    }
    case 0x3F: { // CCF
      if (GET_FLAG(C_FLAG, state.registers)) {
        CLEAR_FLAG(C_FLAG, state.registers);
        SET_FLAG(H_FLAG, state.registers);
      } else {
        SET_FLAG(C_FLAG, state.registers);
        CLEAR_FLAG(H_FLAG, state.registers);
      }
      CLEAR_FLAG(N_FLAG, state.registers);
      cycles = 4;
      break;
    }

    // ------------------------------------------------------------------------
    // INC/DEC 16-bit (BC, DE, HL, SP)
    // ------------------------------------------------------------------------
    case 0x03:
      inc16(state.registers.BC);
      cycles = 6;
      break;
    case 0x0B:
      dec16(state.registers.BC);
      cycles = 6;
      break;
    case 0x13:
      inc16(state.registers.DE);
      cycles = 6;
      break;
    case 0x1B:
      dec16(state.registers.DE);
      cycles = 6;
      break;
    case 0x23:
      inc16(state.registers.HL);
      cycles = 6;
      break;
    case 0x2B:
      dec16(state.registers.HL);
      cycles = 6;
      break;
    case 0x33:
      inc16(state.registers.SP);
      cycles = 6;
      break;
    case 0x3B:
      dec16(state.registers.SP);
      cycles = 6;
      break;

    default:
      // Fall back to catalogue for opcodes not yet migrated
      handled = false;
      break;
    }

    if (handled) {
      tStates += cycles;
      state.addFrameTStates(cycles);
      this->state.tape.update(cycles);
      audio.update(cycles, state.getSpeakerBit(), state.tape.getEarBit());
    } else {
      // Fallback to old catalogue system
      state.registers.PC--; // Rewind PC for catalogue lookup
      OpCode *opCode = getNextInstruction();
      if (opCode != nullptr) {
        // increment past the opcode
        this->state.registers.PC++;

        int cycles = opCode->execute(this->state);
        tStates += cycles;
        state.addFrameTStates(cycles);
        this->state.tape.update(cycles);
        audio.update(cycles, state.getSpeakerBit(), state.tape.getEarBit());

      } else {
        byte unknownOpcode = state.memory[state.registers.PC];
        char errorMsg[100];
        snprintf(errorMsg, sizeof(errorMsg),
                 "Unknown opcode %02X at address %d", unknownOpcode,
                 this->state.registers.PC);
        lastError = std::string(errorMsg);

        debug("Unknown opcode %02X at address %d\n", unknownOpcode,
              this->state.registers.PC);
        running = false;
      }
    }
  }
  audio.flush();

  // Audio Sync: Throttle execution to match audio consumption rate
  // If buffer has > 3 frames of audio (approx 60ms), slow down.
  // This locks emulation speed to the audio card clock (44.1kHz).
  while (audio.getBufferSize() > 2646) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  // Auto-Type Logic (Frame based)
  if (autoLoadTape && running && !paused) {
    frameCounter++;
    // Wait 120 frames (~2.4s) for boot
    if (frameCounter > 120) {
      // Steps:
      // 0: Start J
      // 1: Release J
      // 2: Start Sym+P
      // 3: Release P
      // 4: Start Sym+P
      // 5: Release P
      // 6: Release Sym
      // 7: Start Enter
      // 8: Release Enter
      // 9: Play Tape + End

      keyHoldFrames++;
      const int PRESS_DURATION = 5;
      const int GAP_DURATION = 5;

      switch (autoLoadStep) {
      case 0:                              // J
        state.keyboard.setKey(6, 3, true); // J
        if (keyHoldFrames > PRESS_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 1: // Release J
        state.keyboard.setKey(6, 3, false);
        if (keyHoldFrames > GAP_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 2:                              // Sym+P
        state.keyboard.setKey(7, 1, true); // Sym
        state.keyboard.setKey(5, 0, true); // P
        if (keyHoldFrames > PRESS_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 3: // Release P (keep Sym)
        state.keyboard.setKey(5, 0, false);
        if (keyHoldFrames > GAP_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 4:                              // Sym+P (again for second quote)
        state.keyboard.setKey(5, 0, true); // P
                                           // Sym already held
        if (keyHoldFrames > PRESS_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 5: // Release P
        state.keyboard.setKey(5, 0, false);
        if (keyHoldFrames > GAP_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 6: // Release Sym
        state.keyboard.setKey(7, 1, false);
        if (keyHoldFrames > GAP_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 7: // Enter
        state.keyboard.setKey(6, 0, true);
        if (keyHoldFrames > PRESS_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 8: // Release Enter
        state.keyboard.setKey(6, 0, false);
        if (keyHoldFrames > GAP_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 9: // Play
        state.tape.play();
        autoLoadTape = false;
        break;
      }
    }
  }
}

VideoBuffer *Processor::getVideoBuffer() {
  return state.memory.getVideoBuffer();
}

void Processor::shutdown() {}

void Processor::reset() {
  state.registers.PC = 0x0;
  state.registers.AF = 0xFFFF;
  state.registers.SP = 0xFFFF;
  state.registers.BC = 0;
  state.registers.DE = 0;
  state.registers.HL = 0;
  state.registers.IX = 0;
  state.registers.IY = 0;
  state.registers.I = 0;
  state.registers.R = 0;
  state.setHalted(false);
  state.setInterrupts(false);
  state.setInterruptMode(0); // Reset to IM 0
  lastError = "";
  running = true;
  paused = false;
  audio.reset();
}

void Processor::writeMem(word address, byte value) {
  state.memory.fastWrite(address, value);
}

void Processor::push16(word value) {
  state.registers.SP -= 2;
  // Use SAFE write for stack operations usually, but standard Z80 might allow
  // stack in ROM? No, stack must be in RAM. We use writeMem to ensure we
  // respect memory protections (if any) or just fastWrite. writeMem wraps
  // fastWrite.
  writeMem(state.registers.SP + 1, (value >> 8) & 0xFF);
  writeMem(state.registers.SP, value & 0xFF);
}

word Processor::pop16() {
  byte low = state.memory[state.registers.SP];
  byte high = state.memory[state.registers.SP + 1];
  state.registers.SP += 2;
  return (high << 8) | low;
}

/**
 * Read the next instruction and process it
 * @return
 */
OpCode *Processor::getNextInstruction() {

  byte opcode = state.memory[state.registers.PC];
  return catalogue.lookupOpcode(opcode);
}

// ============================================================================
// Opcode Helper Methods
// ============================================================================

void Processor::add8(byte val) { ALUHelpers::add8(state, val); }

void Processor::adc8(byte val) { ALUHelpers::adc8(state, val); }

void Processor::sub8(byte val) { ALUHelpers::sub8(state, val); }

void Processor::sbc8(byte val) { ALUHelpers::sbc8(state, val); }

void Processor::and8(byte val) { ALUHelpers::and8(state, val); }

void Processor::xor8(byte val) {
  state.registers.A ^= val;
  CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);

  if (state.registers.A == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  if (state.registers.A & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
}

void Processor::or8(byte val) {
  state.registers.A |= val;
  CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);

  if (state.registers.A == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  if (state.registers.A & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
}

void Processor::cp8(byte val) {
  int result = state.registers.A - val;

  if ((result & 0xFF) == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  if (state.registers.A < val)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);

  // S Flag
  if (result & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // N Flag
  SET_FLAG(N_FLAG, state.registers);

  // H Flag
  if ((state.registers.A & 0x0F) < (val & 0x0F))
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  // P/V (Overflow) for CP is same as SUB
  int op1 = (int8_t)state.registers.A;
  int op2 = (int8_t)val;
  int r = (int8_t)(result & 0xFF);
  if (((op1 > 0 && op2 < 0) && r < 0) || ((op1 < 0 && op2 > 0) && r > 0)) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }
}

void Processor::inc8(byte &reg) { ALUHelpers::inc8(state, reg); }

void Processor::dec8(byte &reg) { ALUHelpers::dec8(state, reg); }

// Stubs for extended instructions
void Processor::exec_ed_opcode() {
  // To be implemented
}
void Processor::exec_cb_opcode() {
  // To be implemented
}
void Processor::exec_index_opcode(byte prefix) {
  // To be implemented
}

// 16-bit Stubs
void Processor::add16(word &dest, word src) {
  ALUHelpers::add16(state, dest, src);
}
void Processor::adc16(word &dest, word src) { /* TODO */ }
void Processor::sbc16(word &dest, word src) { /* TODO */ }

void Processor::inc16(word &reg) { ALUHelpers::inc16(state, reg); }

void Processor::dec16(word &reg) { ALUHelpers::dec16(state, reg); }

void Processor::op_load(byte opcode) {
  // To be implemented
}

void Processor::op_arithmetic(byte opcode) {
  // To be implemented
}

void Processor::op_logic(byte opcode) {
  // To be implemented
}

void Processor::op_rotate_shift(byte opcode) {
  // To be implemented
}

void Processor::op_bit(byte opcode) {
  // To be implemented
}

void Processor::op_jump(byte opcode) {
  // To be implemented
}

void Processor::op_stack(byte opcode) {
  // To be implemented
}

void Processor::op_io(byte opcode) {
  // To be implemented
}

void Processor::op_misc(byte opcode) {
  // To be implemented
}
