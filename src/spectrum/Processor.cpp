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
  state.setFastLoad(false); // Default to Slow/Authentic load
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

    case 0x07: // RLCA
    {
      byte val = state.registers.A;
      int carry = (val & 0x80) ? 1 : 0;
      val = (val << 1) | carry;
      state.registers.A = val;

      if (carry)
        SET_FLAG(C_FLAG, state.registers);
      else
        CLEAR_FLAG(C_FLAG, state.registers);

      CLEAR_FLAG(H_FLAG, state.registers);
      CLEAR_FLAG(N_FLAG, state.registers);
      cycles = 4;
      break;
    }

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

    case 0xDD: // IX
      exec_index_opcode(0xDD);
      // exec_index_opcode does its own T-State updates because of complexity
      // so we set cycles=0 here to avoid double counting?
      // My impl logic: "state.addFrameTStates(cycles);" at end of
      // exec_index_opcode. executeFrame loop adds 'tStates += cycles'. If I
      // return void and add internally, executeFrame's 'cycles' local var is 0.
      // lines 1832: "if (handled) { tStates += cycles;
      // state.addFrameTStates(cycles); ... }" If I set handled=true (default),
      // and cycles=0, then `addFrameTStates(0)` adds nothing. But `tStates +=
      // 0` adds nothing. AND my function added usage. BUT `executeFrame` also
      // calls tapes/audio updates. My function calls them. So I should ensure
      // `executeFrame` does NOT duplicate.
      cycles = 0;
      break;

    case 0xFD: // IY
      exec_index_opcode(0xFD);
      cycles = 0;
      break;

    case 0x0F: // RRCA
    {
      byte val = state.registers.A;
      int carry = (val & 0x01) ? 1 : 0;
      val = (val >> 1) | (carry << 7);
      state.registers.A = val;

      if (carry)
        SET_FLAG(C_FLAG, state.registers);
      else
        CLEAR_FLAG(C_FLAG, state.registers);

      CLEAR_FLAG(H_FLAG, state.registers);
      CLEAR_FLAG(N_FLAG, state.registers);
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

    case 0x17: // RLA
    {
      byte val = state.registers.A;
      int oldCarry = GET_FLAG(C_FLAG, state.registers) ? 1 : 0;
      int newCarry = (val & 0x80) ? 1 : 0;
      val = (val << 1) | oldCarry;
      state.registers.A = val;

      if (newCarry)
        SET_FLAG(C_FLAG, state.registers);
      else
        CLEAR_FLAG(C_FLAG, state.registers);

      CLEAR_FLAG(H_FLAG, state.registers);
      CLEAR_FLAG(N_FLAG, state.registers);
      cycles = 4;
      break;
    }

    case 0x18: // JR e
    {
      int8_t offset = (int8_t)m_memory[state.registers.PC];
      state.registers.PC++;
      state.registers.PC += offset;
      cycles = 12;
      break;
    }

    case 0x1F: // RRA
    {
      byte val = state.registers.A;
      int oldCarry = GET_FLAG(C_FLAG, state.registers) ? 1 : 0;
      int newCarry = (val & 0x01) ? 1 : 0;
      val = (val >> 1) | (oldCarry << 7);
      state.registers.A = val;

      if (newCarry)
        SET_FLAG(C_FLAG, state.registers);
      else
        CLEAR_FLAG(C_FLAG, state.registers);

      CLEAR_FLAG(H_FLAG, state.registers);
      CLEAR_FLAG(N_FLAG, state.registers);
      cycles = 4;
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

    case 0xD3: // OUT (n), A
    {
      byte port = state.getNextByteFromPC();
      state.registers.PC++;

      byte value = state.registers.A;

      // Port FE (or any even port on 48K) controls border color and speaker
      if ((port & 0x01) == 0) {
        byte borderColor = state.registers.A & 0x07;
        if (state.memory.getVideoBuffer()) {
          state.memory.getVideoBuffer()->setBorderColor(
              borderColor, state.getFrameTStates());
        }

        state.setSpeakerBit((value & 0x10) != 0);
        state.setMicBit((value & 0x08) != 0);
      }

      cycles = 11;
      break;
    }

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

    case 0xCB: // Prefix CB
      cycles = exec_cb_opcode();
      break;

    case 0xED: // Extended
      cycles = exec_ed_opcode();
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

    case 0xDB: // IN A, (n)
    {
      byte port = state.getNextByteFromPC();
      state.registers.PC++;

      byte highByte = state.registers.A;

      if ((port & 0x01) == 0) {
        byte ear = state.tape.getEarBit() ? 0x40 : 0x00;
        state.registers.A = state.keyboard.readPort(highByte) | ear;
      } else if ((port & 0x1F) == 0x1F) {
        // Kempston Joystick (Port 31)
        state.registers.A = state.keyboard.readKempstonPort();
      } else {
        // Floating bus
        state.registers.A = 0xFF;
      }

      cycles = 11;
      break;
    }

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

    case 0xF9: // LD SP, HL
      state.registers.SP = state.registers.HL;
      cycles = 6;
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

    case 0x27: // DAA
    {
      byte a = state.registers.A;
      byte correction = 0;
      bool n = GET_FLAG(N_FLAG, state.registers);
      bool c = GET_FLAG(C_FLAG, state.registers);
      bool h = GET_FLAG(H_FLAG, state.registers);

      // Wait, DAA flags are specific. sub8/add8 overwrite flags including C, H,
      // N. DAA should preserve N. Set P/V to parity. S, Z from result. And H is
      // diff. Re-doing DAA manually.

      int res = a;
      if (!n) {
        if (h || (a & 0x0F) > 9)
          res += 0x06;
        if (c || (a > 0x9F))
          res += 0x60;
      } else {
        if (h || (a & 0x0F) > 9)
          res -= 0x06;
        if (c || res > 0x99)
          res -= 0x60;
      }

      // Flags
      if (c || (!n && a > 0x99))
        SET_FLAG(C_FLAG, state.registers);
      // Logic for H is complex.
      if ((!n && (a & 0x0F) > 9) || (n && h && (a & 0x0F) < 6))
        SET_FLAG(H_FLAG, state.registers); // Approximate
      else
        CLEAR_FLAG(H_FLAG, state.registers);

      state.registers.A = (byte)res;
      // Parity
      int bits = 0;
      for (int i = 0; i < 8; i++)
        if (res & (1 << i))
          bits++;
      if (bits % 2 == 0)
        SET_FLAG(P_FLAG, state.registers);
      else
        CLEAR_FLAG(P_FLAG, state.registers);
      if ((res & 0xFF) == 0)
        SET_FLAG(Z_FLAG, state.registers);
      else
        CLEAR_FLAG(Z_FLAG, state.registers);
      if (res & 0x80)
        SET_FLAG(S_FLAG, state.registers);
      else
        CLEAR_FLAG(S_FLAG, state.registers);

      cycles = 4;
      break;
    }

    case 0x2F: // CPL
      state.registers.A = ~state.registers.A;
      SET_FLAG(H_FLAG, state.registers);
      SET_FLAG(N_FLAG, state.registers);
      cycles = 4;
      break;

    case 0x37: // SCF
      SET_FLAG(C_FLAG, state.registers);
      CLEAR_FLAG(H_FLAG, state.registers);
      CLEAR_FLAG(N_FLAG, state.registers);
      cycles = 4;
      break;

    case 0xF3: // DI
      state.setInterrupts(false);
      state.registers.IFF1 = 0;
      state.registers.IFF2 = 0;
      cycles = 4;
      break;

    case 0xFB: // EI
      state.setInterrupts(true);
      state.registers.IFF1 = 1;
      state.registers.IFF2 = 1;
      cycles = 4;
      break;

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
int Processor::exec_ed_opcode() {
  byte extOpcode = state.getNextByteFromPC();
  state.registers.PC++; // Increment past the extended opcode

  int cycles = 0;

  switch (extOpcode) {
  // LDS
  case 0x47:
    state.registers.I = state.registers.A;
    cycles = 9;
    break;
  case 0x57:
    state.registers.A = state.registers.I;
    cycles = 9;
    {
      byte f = state.registers.F & C_FLAG;
      if (state.registers.A == 0)
        f |= Z_FLAG;
      if (state.registers.A & 0x80)
        f |= S_FLAG;
      if (state.registers.IFF2)
        f |= P_FLAG;
      state.registers.F = f;
    }
    break;

  // LD A, R / LD R, A
  case 0x4F:
    state.registers.R = state.registers.A;
    cycles = 9;
    break;
  case 0x5F:
    // Preserves bit 7 of R? R is 7 bits refresh.
    // Actually Z80 R register is 8 bits (top bit changes roughly).
    // Emulator often keeps R as 8 bits.
    state.registers.A = state.registers.R;
    cycles = 9;
    {
      byte f = state.registers.F & C_FLAG;
      if (state.registers.A == 0)
        f |= Z_FLAG;
      if (state.registers.A & 0x80)
        f |= S_FLAG;
      if (state.registers.IFF2)
        f |= P_FLAG;
      CLEAR_FLAG(N_FLAG, state.registers);
      CLEAR_FLAG(H_FLAG, state.registers);
      // Merge with existing flags (C preserved)
      // wait, I constructed 'f' based on C_FLAG + new flags.
      // existing code did: byte f = state.registers.F & C_FLAG; ...
      // state.registers.F = f; This CLEARED other flags (like H/N). LD A, I
      // documentation: S, Z, H=0, P/V=IFF2, N=0, C preserved. So code is
      // correct.
      state.registers.F = f;
    }
    break;

  // RRD / RLD
  case 0x67:
    op_ed_rrd();
    cycles = 18;
    break;
  case 0x6F:
    op_ed_rld();
    cycles = 18;
    break;

  // RETI
  case 0x4D: {
    word pc = state.memory[state.registers.SP];
    pc |= (state.memory[state.registers.SP + 1] << 8);
    state.registers.SP += 2;
    state.registers.PC = pc;
    cycles = 14;
  } break;

  // RETN
  case 0x45: {
    word pc = state.memory[state.registers.SP];
    pc |= (state.memory[state.registers.SP + 1] << 8);
    state.registers.SP += 2;
    state.registers.PC = pc;
    state.registers.IFF1 = state.registers.IFF2;
    state.setInterrupts(state.registers.IFF1);
    cycles = 14;
  } break;
  // IN r, (C)
  case 0x40:
    op_ed_in_r_C(state.registers.B);
    cycles = 12;
    break;
  case 0x48:
    op_ed_in_r_C(state.registers.C);
    cycles = 12;
    break;
  case 0x50:
    op_ed_in_r_C(state.registers.D);
    cycles = 12;
    break;
  case 0x58:
    op_ed_in_r_C(state.registers.E);
    cycles = 12;
    break;
  case 0x60:
    op_ed_in_r_C(state.registers.H);
    cycles = 12;
    break;
  case 0x68:
    op_ed_in_r_C(state.registers.L);
    cycles = 12;
    break;
  case 0x78:
    op_ed_in_r_C(state.registers.A);
    cycles = 12;
    break;

  // SBC HL, rr
  case 0x42:
    op_ed_sbc16(state.registers.HL, state.registers.BC);
    cycles = 15;
    break;
  case 0x52:
    op_ed_sbc16(state.registers.HL, state.registers.DE);
    cycles = 15;
    break;
  case 0x62:
    op_ed_sbc16(state.registers.HL, state.registers.HL);
    cycles = 15;
    break;
  case 0x72:
    op_ed_sbc16(state.registers.HL, state.registers.SP);
    cycles = 15;
    break;

  // ADC HL, rr
  case 0x4A:
    op_ed_adc16(state.registers.HL, state.registers.BC);
    cycles = 15;
    break;
  case 0x5A:
    op_ed_adc16(state.registers.HL, state.registers.DE);
    cycles = 15;
    break;
  case 0x6A:
    op_ed_adc16(state.registers.HL, state.registers.HL);
    cycles = 15;
    break;
  case 0x7A:
    op_ed_adc16(state.registers.HL, state.registers.SP);
    cycles = 15;
    break;

  // LD (nn), rr - Consumes 2 byte operand (nn)
  case 0x43:
    op_ed_ld_nn_rr(state.getNextWordFromPC(), state.registers.BC);
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x53:
    op_ed_ld_nn_rr(state.getNextWordFromPC(), state.registers.DE);
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x63:
    op_ed_ld_nn_rr(state.getNextWordFromPC(), state.registers.HL);
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x73:
    op_ed_ld_nn_rr(state.getNextWordFromPC(), state.registers.SP);
    state.registers.PC += 2;
    cycles = 20;
    break;

  // LD rr, (nn) - Consumes 2 byte operand (nn)
  case 0x4B:
    op_ed_ld_rr_nn(state.registers.BC, state.getNextWordFromPC());
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x5B:
    op_ed_ld_rr_nn(state.registers.DE, state.getNextWordFromPC());
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x6B:
    op_ed_ld_rr_nn(state.registers.HL, state.getNextWordFromPC());
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x7B:
    op_ed_ld_rr_nn(state.registers.SP, state.getNextWordFromPC());
    state.registers.PC += 2;
    cycles = 20;
    break;

  // IM
  case 0x46:
    state.setInterruptMode(0);
    cycles = 8;
    break;
  case 0x56:
    state.setInterruptMode(1);
    cycles = 8;
    break;
  case 0x5E:
    state.setInterruptMode(2);
    cycles = 8;
    break;

  // Block Ops
  case 0xB0:
    cycles = op_ed_ldir();
    break;
  case 0xB8:
    cycles = op_ed_lddr();
    break;
  case 0xB1:
    cycles = op_ed_cpir();
    break;
  case 0xB9:
    cycles = op_ed_cpdr();
    break;

  // Block I/O
  case 0xA2:
    cycles = op_ed_ini();
    break;
  case 0xB2:
    cycles = op_ed_inir();
    break;
  case 0xAA:
    cycles = op_ed_ind();
    break;
  case 0xBA:
    cycles = op_ed_indr();
    break;
  case 0xA3:
    cycles = op_ed_outi();
    break;
  case 0xB3:
    cycles = op_ed_otir();
    break;
  case 0xAB:
    cycles = op_ed_outd();
    break;
  case 0xBB:
    cycles = op_ed_otdr();
    break;

  default:
    cycles = 8; // NOP (approx) for unknown ED to prevent infinite loops
    break;
  }
  return cycles;
}
int Processor::exec_cb_opcode() {
  byte cbOpcode = state.getNextByteFromPC();
  state.registers.PC++;

  int x = (cbOpcode >> 6) & 3;
  int y = (cbOpcode >> 3) & 7;
  int z = cbOpcode & 7;

  byte *regPtr = nullptr;
  byte memVal = 0;
  bool isMem = false;
  word hlAddr = state.registers.HL;

  // Decode register
  switch (z) {
  case 0:
    regPtr = &state.registers.B;
    break;
  case 1:
    regPtr = &state.registers.C;
    break;
  case 2:
    regPtr = &state.registers.D;
    break;
  case 3:
    regPtr = &state.registers.E;
    break;
  case 4:
    regPtr = &state.registers.H;
    break;
  case 5:
    regPtr = &state.registers.L;
    break;
  case 6: // (HL)
    isMem = true;
    memVal = state.memory[hlAddr];
    regPtr = &memVal;
    break;
  case 7:
    regPtr = &state.registers.A;
    break;
  }

  int cycles = 8;
  if (isMem)
    cycles = (x == 1) ? 12 : 15; // BIT (HL)=12, others=15
  else
    cycles = 8;

  if (x == 0) { // Rotate/Shift
    switch (y) {
    case 0:
      rlc(*regPtr);
      break;
    case 1:
      rrc(*regPtr);
      break;
    case 2:
      rl(*regPtr);
      break;
    case 3:
      rr(*regPtr);
      break;
    case 4:
      sla(*regPtr);
      break;
    case 5:
      sra(*regPtr);
      break;
    case 6:
      sll(*regPtr);
      break; // SLL (undocumented)
    case 7:
      srl(*regPtr);
      break;
    }
  } else if (x == 1) { // BIT
    bit(y, *regPtr);
    // BIT doesn't write back
    if (isMem) {
      return cycles;
    }
  } else if (x == 2) { // RES
    res(y, *regPtr);
  } else if (x == 3) { // SET
    set(y, *regPtr);
  }

  // Write back if memory and NOT BIT (BIT doesn't modify)
  if (isMem && x != 1) {
    writeMem(hlAddr, *regPtr);
  }

  return cycles;
}
void Processor::exec_index_opcode(byte prefix) {
  // Determine Index Register
  word &idx = (prefix == 0xDD) ? state.registers.IX : state.registers.IY;

  // Fetch opcode
  byte opcode = state.getNextByteFromPC();
  state.registers.PC++;

  // Handle DD CB d <opcode>
  if (opcode == 0xCB) {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    byte cbOp = state.getNextByteFromPC();
    state.registers.PC++;

    word addr = idx + (int8_t)d;
    byte val = state.memory[addr];

    int x = (cbOp >> 6) & 3;
    int y = (cbOp >> 3) & 7;
    int z = cbOp & 7;

    // Most CB ops operate on 'val' then write back
    // BIT (x=1) does not write back.
    // Logic is similar to standard CB but on (IX+d).
    // Note: Z80 undocumented behavior: result is ALSO copied to register
    // specified by 'z' (if not 6). But official behavior only updates (IX+d).
    // We'll stick to official for now.

    int cycles = 23; // Usually 23 for indexed bit ops

    if (x == 0) { // Rotate/Shift
      switch (y) {
      case 0:
        rlc(val);
        break;
      case 1:
        rrc(val);
        break;
      case 2:
        rl(val);
        break;
      case 3:
        rr(val);
        break;
      case 4:
        sla(val);
        break;
      case 5:
        sra(val);
        break;
      case 6:
        sll(val);
        break;
      case 7:
        srl(val);
        break;
      }
      writeMem(addr, val);
    } else if (x == 1) { // BIT
      bit(y, val);
      cycles = 20;       // BIT is 20
                         // No writeback
    } else if (x == 2) { // RES
      res(y, val);
      writeMem(addr, val);
    } else if (x == 3) { // SET
      set(y, val);
      writeMem(addr, val);
    }

    state.addFrameTStates(cycles);
    this->state.tape.update(cycles);
    audio.update(cycles, state.getSpeakerBit(), state.tape.getEarBit());
    return;
  }

  // Standard Index Opcodes
  // Helper for High/Low byte access (Little Endian host assumed)
  byte *idxL = (byte *)&idx;
  byte *idxH = (byte *)&idx + 1;

  int cycles = 0;

  switch (opcode) {
  // 09, 19, 29, 39: ADD IX, rr
  case 0x09:
    add16(idx, state.registers.BC);
    cycles = 15;
    break;
  case 0x19:
    add16(idx, state.registers.DE);
    cycles = 15;
    break;
  case 0x29:
    add16(idx, idx);
    cycles = 15;
    break;
  case 0x39:
    add16(idx, state.registers.SP);
    cycles = 15;
    break;

  // 21 nn: LD IX, nn
  case 0x21: {
    idx = state.getNextWordFromPC();
    state.registers.PC += 2;
    cycles = 14;
    break;
  }

  // 22 nn: LD (nn), IX
  case 0x22: {
    word addr = state.getNextWordFromPC();
    state.registers.PC += 2;
    writeMem(addr, *idxL);     // Low byte
    writeMem(addr + 1, *idxH); // High byte
    cycles = 20;
    break;
  }

  // 2A nn: LD IX, (nn)
  case 0x2A: {
    word addr = state.getNextWordFromPC();
    state.registers.PC += 2;
    *idxL = state.memory[addr];
    *idxH = state.memory[addr + 1];
    cycles = 20;
    break;
  }

  // 23: INC IX
  case 0x23:
    idx++;
    cycles = 10;
    break;

  // 2B: DEC IX
  case 0x2B:
    idx--;
    cycles = 10;
    break;

  // 24: INC IXH
  case 0x24: {
    ALUHelpers::inc8(state, *idxH);
    cycles = 8;
    break;
  }
  // 25: DEC IXH
  case 0x25: {
    ALUHelpers::dec8(state, *idxH);
    cycles = 8;
    break;
  }
  // 2C: INC IXL
  case 0x2C: {
    ALUHelpers::inc8(state, *idxL);
    cycles = 8;
    break;
  }
  // 2D: DEC IXL
  case 0x2D: {
    ALUHelpers::dec8(state, *idxL);
    cycles = 8;
    break;
  }

  // 34: INC (IX+d)
  case 0x34: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    word addr = idx + (int8_t)d;
    byte val = state.memory[addr];
    // Use helper for correct flags (H, P/V)
    ALUHelpers::inc8(state, val);
    writeMem(addr, val);
    cycles = 23;
    break;
  }

  // 35: DEC (IX+d)
  case 0x35: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    word addr = idx + (int8_t)d;
    byte val = state.memory[addr];
    // Use helper for correct flags (H, P/V)
    ALUHelpers::dec8(state, val);
    writeMem(addr, val);
    cycles = 23;
    break;
  }

  // 36: LD (IX+d), n
  case 0x36: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    byte n = state.getNextByteFromPC();
    state.registers.PC++;
    writeMem(idx + (int8_t)d, n);
    cycles = 19;
    break;
  }

  // UNDOCUMENTED / REQUESTED OPCODES
  // 44: LD B, IXH/IYH
  case 0x44:
    state.registers.B = *idxH;
    cycles = 8;
    break;
  // 45: LD B, IXL/IYL -- standard mapping LD B,L -> LD B, IXL
  case 0x45:
    state.registers.B = *idxL;
    cycles = 8;
    break;

  // 4C: LD C, IXH
  case 0x4C:
    state.registers.C = *idxH;
    cycles = 8;
    break;
  // 4D: LD C, IXL (Requested)
  case 0x4D:
    state.registers.C = *idxL;
    cycles = 8;
    break;

  // 54: LD D, IXH
  case 0x54:
    state.registers.D = *idxH;
    cycles = 8;
    break;
  // 55: LD D, IXL
  case 0x55:
    state.registers.D = *idxL;
    cycles = 8;
    break;

  // 5C: LD E, IXH
  case 0x5C:
    state.registers.E = *idxH;
    cycles = 8;
    break;
  // 5D: LD E, IXL
  case 0x5D:
    state.registers.E = *idxL;
    cycles = 8;
    break;

  // 60: LD IXH, B
  case 0x60:
    *idxH = state.registers.B;
    cycles = 8;
    break;
  // 61: LD IXH, C
  case 0x61:
    *idxH = state.registers.C;
    cycles = 8;
    break;
  // 62: LD IXH, D
  case 0x62:
    *idxH = state.registers.D;
    cycles = 8;
    break;
  // 63: LD IXH, E
  case 0x63:
    *idxH = state.registers.E;
    cycles = 8;
    break;
  // 64: LD IXH, IXH (NOP)
  case 0x64:
    cycles = 8;
    break;
  // 65: LD IXH, IXL
  case 0x65:
    *idxH = *idxL;
    cycles = 8;
    break;

  // 67: LD IXH, A (Undocumented)
  case 0x67:
    *idxH = state.registers.A;
    cycles = 8;
    break;

  // 68: LD IXL, B
  case 0x68:
    *idxL = state.registers.B;
    cycles = 8;
    break;
  // 69: LD IXL, C
  case 0x69:
    *idxL = state.registers.C;
    cycles = 8;
    break;
  // 6A: LD IXL, D (Undocumented)
  case 0x6A:
    *idxL = state.registers.D;
    cycles = 8;
    break;
  // 6B: LD IXL, E (Undocumented)
  case 0x6B:
    *idxL = state.registers.E;
    cycles = 8;
    break;

  // 6C: LD IXL, H -> LD IXL, IXH
  case 0x6C:
    *idxL = *idxH;
    cycles = 8;
    break;
  // 6D: LD IXL, L -> LD IXL, IXL
  case 0x6D:
    cycles = 8;
    break;

  // 6F: LD IXL, A (Undocumented)
  case 0x6F:
    *idxL = state.registers.A;
    cycles = 8;
    break;

  // 70-75, 77: LD (IX+d), r
  case 0x70: { // LD (IX+d), B
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    writeMem(idx + (int8_t)d, state.registers.B);
    cycles = 19;
    break;
  }
  case 0x71: { // LD (IX+d), C
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    writeMem(idx + (int8_t)d, state.registers.C);
    cycles = 19;
    break;
  }
  case 0x72: { // LD (IX+d), D
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    writeMem(idx + (int8_t)d, state.registers.D);
    cycles = 19;
    break;
  }
  case 0x73: { // LD (IX+d), E
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    writeMem(idx + (int8_t)d, state.registers.E);
    cycles = 19;
    break;
  }
  case 0x74: { // LD (IX+d), H
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    writeMem(idx + (int8_t)d, state.registers.H);
    cycles = 19;
    break;
  }
  case 0x75: { // LD (IX+d), L
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    writeMem(idx + (int8_t)d, state.registers.L);
    cycles = 19;
    break;
  }
  case 0x77: { // LD (IX+d), A
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    writeMem(idx + (int8_t)d, state.registers.A);
    cycles = 19;
    break;
  }

  // 7C: LD A, IXH
  case 0x7C:
    state.registers.A = *idxH;
    cycles = 8;
    break;
  // 7D: LD A, IXL
  case 0x7D:
    state.registers.A = *idxL;
    cycles = 8;
    break;

  // 7E: LD A, (IX+d)
  case 0x7E: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    state.registers.A = state.memory[idx + (int8_t)d];
    cycles = 19;
    break;
  }

  // ALU (IX+d)
  case 0x86: { // ADD A, (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    ALUHelpers::add8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0x8E: { // ADC A, (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    ALUHelpers::adc8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0x96: { // SUB (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    ALUHelpers::sub8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0x9E: { // SBC A, (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    ALUHelpers::sbc8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0xA6: { // AND (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    ALUHelpers::and8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0xAE: { // XOR (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    ALUHelpers::xor8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0xB6: { // OR (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    ALUHelpers::or8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0xBE: { // CP (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    ALUHelpers::cp8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  // 46: LD B, (IX+d)
  case 0x46: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    state.registers.B = state.memory[idx + (int8_t)d];
    cycles = 19;
    break;
  }
  // 4E, 56, 5E, 66, 6E
  case 0x4E: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    state.registers.C = state.memory[idx + (int8_t)d];
    cycles = 19;
    break;
  }
  case 0x56: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    state.registers.D = state.memory[idx + (int8_t)d];
    cycles = 19;
    break;
  }
  case 0x5E: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    state.registers.E = state.memory[idx + (int8_t)d];
    cycles = 19;
    break;
  }
  case 0x66: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    state.registers.H = state.memory[idx + (int8_t)d];
    cycles = 19;
    break;
  }
  case 0x6E: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    state.registers.L = state.memory[idx + (int8_t)d];
    cycles = 19;
    break;
  }

  // B4: OR IXH (Requested)
  case 0xB4: {
    ALUHelpers::or8(state, *idxH);
    cycles = 8;
    break;
  }
  // B5: OR IXL (Requested)
  case 0xB5: {
    ALUHelpers::or8(state, *idxL);
    cycles = 8;
    break;
  }

  // E1: POP IX
  case 0xE1: {
    idx = pop16();
    cycles = 14;
    break;
  }
  // E5: PUSH IX
  case 0xE5: {
    push16(idx);
    cycles = 15;
    break;
  }

  // E3: EX (SP), IX
  case 0xE3: {
    byte low = state.memory[state.registers.SP];
    byte high = state.memory[state.registers.SP + 1];
    writeMem(state.registers.SP, idx & 0xFF);
    writeMem(state.registers.SP + 1, (idx >> 8) & 0xFF);
    idx = (high << 8) | low;
    cycles = 23;
    break;
  }

  // E9: JP (IX)
  case 0xE9: {
    state.registers.PC = idx;
    cycles = 8;
    break;
  }

  // F9: LD SP, IX
  case 0xF9: {
    state.registers.SP = idx;
    cycles = 10;
    break;
  }

  default:

    // Handle unhandled index ops as standard opcodes with no displacement?
    // Or if standard op uses HL, use IX?
    // Z80 Rule: If opcode not index-aware, execute as standard.
    // If it accessed (HL), it accesses (IX+d).
    // BUT my switch handles (IX+d) cases explicitly (7E, 46, etc).
    // What if it's ADD A, (IX+d) (86)?
    // I need to implement ALU ops!
    // 86: ADD A, (IX+d)
    // 96: SUB (IX+d)
    // A6: AND (IX+d)
    // B6: OR (IX+d)
    // BE: CP (IX+d)
    // These are ESSENTIAL.
    state.registers
        .PC--; // Rewind? No, standard dispatch in executeFrame is separated.
    // Debugging loop:
    printf("Unknown Index Opcode %02X %02X\n", prefix, opcode);
    cycles = 4;
    break;
  }

  state.addFrameTStates(cycles);
  this->state.tape.update(cycles);
  audio.update(cycles, state.getSpeakerBit(), state.tape.getEarBit());
}

// 16-bit Stubs
void Processor::add16(word &dest, word src) {
  ALUHelpers::add16(state, dest, src);
}
void Processor::inc16(word &reg) { ALUHelpers::inc16(state, reg); }
void Processor::dec16(word &reg) { ALUHelpers::dec16(state, reg); }

// Bit Manipulation Helpers
void Processor::rlc(byte &val) {
  int carry = (val & 0x80) ? 1 : 0;
  val = (val << 1) | carry;
  // state.registers.F = (carry ? C_FLAG : 0) | (val & S_FLAG) | (val == 0 ?
  // Z_FLAG : 0) | (state.registers.F & ~(C_FLAG | S_FLAG | Z_FLAG | H_FLAG |
  // N_FLAG));

  if (carry)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
}

void Processor::rrc(byte &val) {
  int carry = (val & 0x01) ? 1 : 0;
  val = (val >> 1) | (carry << 7);

  if (carry)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
}

void Processor::rl(byte &val) {
  int oldCarry = GET_FLAG(C_FLAG, state.registers) ? 1 : 0;
  int newCarry = (val & 0x80) ? 1 : 0;
  val = (val << 1) | oldCarry;

  if (newCarry)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
}

void Processor::rr(byte &val) {
  int oldCarry = GET_FLAG(C_FLAG, state.registers) ? 1 : 0;
  int newCarry = (val & 0x01) ? 1 : 0;
  val = (val >> 1) | (oldCarry << 7);

  if (newCarry)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
}

void Processor::sla(byte &val) {
  int carry = (val & 0x80) ? 1 : 0;
  val = val << 1;

  if (carry)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
}

void Processor::sra(byte &val) {
  int carry = (val & 0x01) ? 1 : 0;
  int msb = val & 0x80;
  val = (val >> 1) | msb;

  if (carry)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
}

void Processor::sll(byte &val) {
  // SLL (Undocumented): Shift Left Logical, inserts 1 into bit 0
  int carry = (val & 0x80) ? 1 : 0;
  val = (val << 1) | 0x01;

  if (carry)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
}

void Processor::srl(byte &val) {
  int carry = (val & 0x01) ? 1 : 0;
  val = val >> 1;

  if (carry)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
}

void Processor::bit(int bit, byte val) {
  bool z = !((val >> bit) & 1);
  if (z)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  SET_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
}

void Processor::set(int bit, byte &val) { val |= (1 << bit); }
void Processor::res(int bit, byte &val) { val &= ~(1 << bit); }

// Extended Helpers
void Processor::op_ed_ld_nn_rr(word nn, word rr) {
  // LD (nn), rr. Z80 writes Low byte to nn, High byte to nn+1.
  writeMem(nn, (byte)(rr & 0xFF));
  writeMem(nn + 1, (byte)((rr >> 8) & 0xFF));
}

void Processor::op_ed_ld_rr_nn(word &rr, word nn) {
  // LD rr, (nn)
  byte low = state.memory[nn];
  byte high = state.memory[nn + 1];
  rr = (word)(high << 8) | low;
}

void Processor::op_ed_in_r_C(byte &r) {
  // IN r, (C)
  // Results often placed in r. Flags affected.
  // Port address is BC. High byte (B) selects the keyboard row.

  // We must pass the High Byte to readPort to select the correct row.
  byte val = state.keyboard.readPort(state.registers.B);

  // If scanning the ULA (Port FE etc, usually Bit 0 is 0), we need to add EAR
  // bit? The ULA responds to any even port number. Port address low byte is C.
  if ((state.registers.C & 1) == 0) {
    byte ear = state.tape.getEarBit() ? 0x40 : 0x00;
    val |= ear;
  }

  // Note: real Z80 IN r, (C) behaviour puts data on bus
  r = val;

  // Flags: S, Z, H=0, P/V (parity), N=0
  if (val & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  // P/V for IN is Parity
  int p = 0;
  for (int i = 0; i < 8; i++) {
    if (val & (1 << i))
      p++;
  }
  if ((p % 2) == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

void Processor::op_ed_out_C_r(byte r) {
  // OUT (C), r
  // state.registers.B is high byte of port, C is low
  word port = (state.registers.B << 8) | state.registers.C;
  // Perform IO
  // Note: Existing IO logic handles port specific checks
  if ((port & 0xFF) == 0xFE) {
    // ULA
    // Border etc
  }
}

void Processor::op_ed_sbc16(word &dest, word src) {
  // SBC HL, rr
  long val = dest - src - (GET_FLAG(C_FLAG, state.registers) ? 1 : 0);
  int H_carry = (((dest & 0x0FFF) - (src & 0x0FFF) -
                  (GET_FLAG(C_FLAG, state.registers) ? 1 : 0)) < 0);

  if ((val & 0xFFFF0000) != 0)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  SET_FLAG(N_FLAG, state.registers);
  if (val == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (H_carry)
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);
  if (val & 0x8000)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // Overflow (P/V) for SUB/SBC
  short op1 = (short)dest;
  short op2 = (short)src;
  short r = (short)val;
  if (((op1 > 0 && op2 < 0) && r < 0) || ((op1 < 0 && op2 > 0) && r > 0)) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  dest = (word)val;
}

void Processor::op_ed_adc16(word &dest, word src) {
  // ADC HL, rr
  long val = dest + src + (GET_FLAG(C_FLAG, state.registers) ? 1 : 0);
  // H flag (from bit 11 to 12)
  int H_carry = (((dest & 0x0FFF) + (src & 0x0FFF) +
                  (GET_FLAG(C_FLAG, state.registers) ? 1 : 0)) > 0x0FFF);

  if (val > 0xFFFF)
    SET_FLAG(C_FLAG, state.registers);
  else
    CLEAR_FLAG(C_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  if ((val & 0xFFFF) == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  if (H_carry)
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);
  if (val & 0x8000)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // Overflow (P/V)
  short op1 = (short)dest;
  short op2 = (short)src;
  short r = (short)val;
  if (((op1 > 0 && op2 > 0) && r < 0) || ((op1 < 0 && op2 < 0) && r > 0)) {
    SET_FLAG(P_FLAG, state.registers);
  } else {
    CLEAR_FLAG(P_FLAG, state.registers);
  }

  dest = (word)val;
}

// Block Ops - return cycle count
int Processor::op_ed_ldir() {
  byte value = state.memory[state.registers.HL];
  writeMem(state.registers.DE, value);
  state.registers.DE++;
  state.registers.HL++;
  state.registers.BC--;

  // Flags
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(P_FLAG, state.registers);

  if (state.registers.BC != 0) {
    state.registers.PC -= 2;
    return 21;
  } else {
    return 16;
  }
}

int Processor::op_ed_lddr() {
  byte value = state.memory[state.registers.HL];
  writeMem(state.registers.DE, value);
  state.registers.DE--;
  state.registers.HL--;
  state.registers.BC--;

  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);
  CLEAR_FLAG(P_FLAG, state.registers);

  if (state.registers.BC != 0) {
    state.registers.PC -= 2;
    return 21;
  } else {
    return 16;
  }
}

int Processor::op_ed_cpir() {
  // Compare A with (HL), HL++, BC--
  byte value = state.memory[state.registers.HL];
  int result = state.registers.A - value;

  bool z = (result == 0);
  if (z)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  SET_FLAG(N_FLAG, state.registers); // CP sets N

  // S Flag
  if (result & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // H Flag
  if ((state.registers.A & 0x0F) < (value & 0x0F))
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  state.registers.HL++;
  state.registers.BC--;

  bool bcNonZero = (state.registers.BC != 0);
  if (bcNonZero)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers); // P/V indicates BC!=0

  // If BC!=0 AND !Z, repeat
  if (bcNonZero && !z) {
    state.registers.PC -= 2;
    return 21;
  } else {
    return 16;
  }
}

int Processor::op_ed_cpdr() {
  // Compare A with (HL), HL--, BC--
  byte value = state.memory[state.registers.HL];
  int result = state.registers.A - value;

  bool z = (result == 0);
  if (z)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  SET_FLAG(N_FLAG, state.registers); // CP sets N

  // S Flag
  if (result & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);

  // H Flag
  if ((state.registers.A & 0x0F) < (value & 0x0F))
    SET_FLAG(H_FLAG, state.registers);
  else
    CLEAR_FLAG(H_FLAG, state.registers);

  state.registers.HL--;
  state.registers.BC--;

  bool bcNonZero = (state.registers.BC != 0);
  if (bcNonZero)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers); // P/V indicates BC!=0

  // If BC!=0 AND !Z, repeat
  if (bcNonZero && !z) {
    state.registers.PC -= 2;
    return 21;
  } else {
    return 16;
  }
}
// INI
int Processor::op_ed_ini() {
  byte b = state.registers.B;
  byte val = state.keyboard.readPort(b); // Or generic readPort?
  // Processor::op_ed_in_r_C uses keyboard.readPort logic but generalized
  // For Block I/O, it's generic port read using BC?
  // Actually INI uses generic IO port read.
  // My op_ed_in_r_C had specific keyboard/EAR logic.
  // I should use a generic readIO(port) helper?
  // For now, I'll copy the logic from in_r_C but adapt.
  // Wait, op_ed_in_r_C uses state.keyboard.readPort(state.registers.B)
  // because keyboard scan uses high byte.
  // Generic IO uses BC.
  // Let's implement minimal IO here.

  // INI logic:
  // (HL) <- IN(BC)
  // Note: B is decremented AFTER read or before?
  // Z80 manual: "The contents of C are placed on address bus A0...A7, contents
  // of B on A8...A15. I/O read." THEN B is decremented.

  writeMem(state.registers.HL, val);

  state.registers.HL++;
  state.registers.B--;

  SET_FLAG(N_FLAG, state.registers);
  if (state.registers.B == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  return 16;
}

// INIR
int Processor::op_ed_inir() {
  op_ed_ini();
  if (state.registers.B != 0) {
    state.registers.PC -= 2;
    return 21;
  }
  return 16;
}

// IND
int Processor::op_ed_ind() {
  byte b = state.registers.B;
  byte val = state.keyboard.readPort(b);
  writeMem(state.registers.HL, val);

  state.registers.HL--;
  state.registers.B--;

  SET_FLAG(N_FLAG, state.registers);
  if (state.registers.B == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  return 16;
}

// INDR
int Processor::op_ed_indr() {
  op_ed_ind();
  if (state.registers.B != 0) {
    state.registers.PC -= 2;
    return 21;
  }
  return 16;
}

// OUTI
int Processor::op_ed_outi() {
  byte val = state.memory[state.registers.HL];
  state.registers.B--; // Pre-dec B?
  // Z80 manual: "The contents of HL are placed on the data bus... The contents
  // of C are placed on lower address bus... B is decremented... contents of B
  // placed on upper address bus... Output." So B is decremented BEFORE output
  // address is formed? "B is decremented". "The byte from (HL) is written to
  // port (C)". Wait, "IO write". Port address is BC? "The contents of the B
  // register are placed on the top half of the address bus". AFTER decrement.
  // So OUTI: B--; OUT(BC), val; HL++;

  // Wait, INI decrements B AFTER read.
  // OUTI decrements B BEFORE write?
  // Implementation in other emulators:
  // INI: (HL)=IN(BC); Dec B, Inc HL.
  // OUTI: B--; OUT(BC)=(HL); Inc HL.

  // Let's assume standard behavior:
  // INI: Read, WriteMem, Dec B, Inc HL.
  // OUTI: ReadMem, Dec B, Output, Inc HL.

  // Check INI again: Z80 manual says "The contents of Register B are placed on
  // the top half... Then B is decremented". So INI uses OLD B.

  // OUTI: "The contents of Register B are decremented... Then the contents of B
  // are placed on the top half..." So OUTI uses NEW B.

  // OUTI implementation:
  // Read (HL)
  state.registers.B--;
  // Output to Port BC (New B)
  // For now, minimal output logic (stubbed IO)
  // Emulate port contention/logic if needed, but here just cycle count matters
  // often.

  state.registers.HL++;

  SET_FLAG(N_FLAG, state.registers);
  if (state.registers.B == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  return 16;
}

// OTIR
int Processor::op_ed_otir() {
  op_ed_outi();
  if (state.registers.B != 0) {
    state.registers.PC -= 2;
    return 21;
  }
  return 16;
}

// OUTD
int Processor::op_ed_outd() {
  byte val = state.memory[state.registers.HL];
  state.registers.B--;
  // Output val to BC
  state.registers.HL--;

  SET_FLAG(N_FLAG, state.registers);
  if (state.registers.B == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);

  return 16;
}

// OTDR
int Processor::op_ed_otdr() {
  op_ed_outd();
  if (state.registers.B != 0) {
    state.registers.PC -= 2;
    return 21;
  }
  return 16;
}

void Processor::op_ed_rrd() {
  byte hlVal = state.memory[state.registers.HL];
  byte a = state.registers.A;

  // A input: . . . . 3 2 1 0
  // HL input: 7 6 5 4 3 2 1 0

  // RRD:
  // A(0-3) -> HL(0-3)
  // HL(0-3) -> HL(4-7)
  // HL(4-7) -> A(0-3)

  byte newA = (a & 0xF0) | (hlVal & 0x0F);
  byte newHL = ((hlVal >> 4) & 0x0F) | ((a & 0x0F) << 4);

  /* Wait, RRD definition:
     Rotate Right Decimal.
     The 4 bits 0-3 of (HL) are shifted to 0-3 of A.
     The 4 bits 0-3 of A are shifted to 4-7 of (HL).
     The 4 bits 4-7 of (HL) are shifted to 0-3 of (HL).

     Let's trace:
     A:  [Ahi Alo]
     HL: [Hhi Hlo]

     Result:
     A:  [Ahi Hlo]
     HL: [Alo Hhi]

     Documentation says:
     "The contents of the low order four bits of the memory location (HL) are
     copied into the low order four bits of the Accumulator. The previous
     contents of the low order four bits of the Accumulator are copied into the
     high order four bits of (HL). The previous contents of the high order four
     bits of (HL) are copied into the low order four bits of (HL)."

     So:
     A(0-3) = old HL(0-3)
     HL(4-7) = old A(0-3)
     HL(0-3) = old HL(4-7)
  */

  byte oldA = state.registers.A;
  byte oldHL = state.memory[state.registers.HL];

  byte finalA = (oldA & 0xF0) | (oldHL & 0x0F);
  byte finalHL = ((oldA & 0x0F) << 4) | ((oldHL >> 4) & 0x0F);

  state.registers.A = finalA;
  writeMem(state.registers.HL, finalHL);

  // Flags
  // S, Z, P/V set by A output
  // H, N = 0
  if (finalA & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
  if (finalA == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);

  // P/V Parity of A
  int p = 0;
  for (int i = 0; i < 8; i++)
    if (finalA & (1 << i))
      p++;
  if ((p % 2) == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}

void Processor::op_ed_rld() {
  /* RLD:
     Rotate Left Decimal.
     A(0-3) -> HL(4-7) -> HL(0-3) -> A(0-3) is wrong direction.

     Doc:
     "The contents of the low order four bits of the Accumulator are copied into
     the low order four bits of (HL). The previous contents of the low order
     four bits of (HL) are copied into the high order four bits of (HL). The
     previous contents of the high order four bits of (HL) are copied into the
     low order four bits of the Accumulator."

     A:  [Ahi Alo]
     HL: [Hhi Hlo]

     A(0-3) = old HL(4-7)
     HL(4-7) = old HL(0-3)
     HL(0-3) = old A(0-3)
  */

  byte oldA = state.registers.A;
  byte oldHL = state.memory[state.registers.HL];

  byte finalA = (oldA & 0xF0) | ((oldHL >> 4) & 0x0F);
  byte finalHL = ((oldHL & 0x0F) << 4) | (oldA & 0x0F);

  state.registers.A = finalA;
  writeMem(state.registers.HL, finalHL);

  // Flags same as RRD
  if (finalA & 0x80)
    SET_FLAG(S_FLAG, state.registers);
  else
    CLEAR_FLAG(S_FLAG, state.registers);
  if (finalA == 0)
    SET_FLAG(Z_FLAG, state.registers);
  else
    CLEAR_FLAG(Z_FLAG, state.registers);
  CLEAR_FLAG(H_FLAG, state.registers);
  CLEAR_FLAG(N_FLAG, state.registers);

  int p = 0;
  for (int i = 0; i < 8; i++)
    if (finalA & (1 << i))
      p++;
  if ((p % 2) == 0)
    SET_FLAG(P_FLAG, state.registers);
  else
    CLEAR_FLAG(P_FLAG, state.registers);
}
