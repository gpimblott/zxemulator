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
// #include "ALUHelpers.h" // Removed
#include "ProcessorMacros.h"
#include "SnapshotLoader.h"
#include "instructions/ArithmeticInstructions.h"
#include "instructions/BitInstructions.h"
#include "instructions/ControlInstructions.h"
#include "instructions/IOInstructions.h"
#include "instructions/LoadInstructions.h"
#include "instructions/LogicInstructions.h"
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

bool Processor::handleInterrupts(int &tStates) {
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
    return true;
  }
  return false;
}

bool Processor::handleFastLoad() {
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
    return true;
  }
  return false;
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
  // Fire an interrupt
  if (handleInterrupts(tStates)) {
    // Interrupt fired
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
    // Fast Load Trap
    if (handleFastLoad()) {
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
      Bit::rlca(state);
      cycles = 4;
      break;

    case 0x08: // EX AF, AF'
      Load::ex_af_af(state);
      cycles = 4;
      break;

    case 0xD9: // EXX
      Load::exx(state);
      cycles = 4;
      break;

    case 0xDD: // IX
      cycles = exec_index_opcode(0xDD);
      break;

    case 0xFD: // IY
      cycles = exec_index_opcode(0xFD);
      break;

    case 0x0F: // RRCA
      Bit::rrca(state);
      cycles = 4;
      break;

    case 0x10: // DJNZ e
      cycles = Control::djnz(state, (int8_t)state.getNextByteFromPC());
      break;

    // RST instructions
    case 0xC7: // RST 00
      cycles = Control::rst(state, 0x0000);
      break;

    case 0xCF: // RST 08
      cycles = Control::rst(state, 0x0008);
      break;

    case 0xD7: // RST 10
      cycles = Control::rst(state, 0x0010);
      break;

    case 0xDF: // RST 18
      cycles = Control::rst(state, 0x0018);
      break;

    case 0xE7: // RST 20
      cycles = Control::rst(state, 0x0020);
      break;

    case 0xEF: // RST 28
      cycles = Control::rst(state, 0x0028);
      break;

    case 0xF7: // RST 30
      cycles = Control::rst(state, 0x0030);
      break;

    case 0xFF: // RST 38
      cycles = Control::rst(state, 0x0038);
      break;

    case 0x17: // RLA
      Bit::rla(state);
      cycles = 4;
      break;

    case 0x18: // JR e
      cycles = Control::jr(state, (int8_t)state.getNextByteFromPC());
      break;

    case 0x1F: // RRA
      Bit::rra(state);
      cycles = 4;
      break;

    case 0x20: // JR NZ, e
      cycles = Control::jr_cond(state, !GET_FLAG(Z_FLAG, state.registers),
                                (int8_t)state.getNextByteFromPC());
      break;

    case 0x28: // JR Z, e
      cycles = Control::jr_cond(state, GET_FLAG(Z_FLAG, state.registers),
                                (int8_t)state.getNextByteFromPC());
      break;

    case 0x30: // JR NC, e
      cycles = Control::jr_cond(state, !GET_FLAG(C_FLAG, state.registers),
                                (int8_t)state.getNextByteFromPC());
      break;

    case 0x38: // JR C, e
      cycles = Control::jr_cond(state, GET_FLAG(C_FLAG, state.registers),
                                (int8_t)state.getNextByteFromPC());
      break;

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

    // 16-bit Load
    case 0x21: { // LD HL, nn
      word nn = state.getNextWordFromPC();
      state.registers.PC += 2;
      state.registers.HL = nn;
      cycles = 10;
      break;
    }

    case 0x22: { // LD (nn), HL
      word addr = state.getNextWordFromPC();
      state.registers.PC += 2;
      writeMem(addr, state.registers.L);
      writeMem(addr + 1, state.registers.H);
      cycles = 16;
      break;
    }

    case 0x2A: { // LD HL, (nn)
      word addr = state.getNextWordFromPC();
      state.registers.PC += 2;
      state.registers.L = m_memory[addr];
      state.registers.H = m_memory[addr + 1];
      cycles = 16;
      break;
    }

    // ------------------------------------------------------------------------
    // 16-bit Arithmetic
    // ------------------------------------------------------------------------
    case 0x09: // ADD HL, BC
      Arithmetic::add16(state, state.registers.HL, state.registers.BC);
      cycles = 11;
      break;
    case 0x19: // ADD HL, DE
      Arithmetic::add16(state, state.registers.HL, state.registers.DE);
      cycles = 11;
      break;
    case 0x29: // ADD HL, HL
      Arithmetic::add16(state, state.registers.HL, state.registers.HL);
      cycles = 11;
      break;
    case 0x39: // ADD HL, SP
      Arithmetic::add16(state, state.registers.HL, state.registers.SP);
      cycles = 11;
      break;

    case 0xC3: // JP nn
      cycles = Control::jp(state, state.getNextWordFromPC());
      break;

    case 0xE9: // JP (HL)
      cycles = Control::jp_hl(state);
      break;

    // Conditional Jumps
    case 0xC2: // JP NZ, nn
      cycles = Control::jp_cond(state, !GET_FLAG(Z_FLAG, state.registers),
                                state.getNextWordFromPC());
      break;

    case 0xCA: // JP Z, nn
      cycles = Control::jp_cond(state, GET_FLAG(Z_FLAG, state.registers),
                                state.getNextWordFromPC());
      break;

    case 0xD2: // JP NC, nn
      cycles = Control::jp_cond(state, !GET_FLAG(C_FLAG, state.registers),
                                state.getNextWordFromPC());
      break;

    case 0xD3: { // OUT (n), A
      byte port = state.getNextByteFromPC();
      state.registers.PC++;
      cycles = IO::out_n_a(state, port);
      break;
    }

    case 0xDA: // JP C, nn
      cycles = Control::jp_cond(state, GET_FLAG(C_FLAG, state.registers),
                                state.getNextWordFromPC());
      break;

    case 0xE2: // JP PO, nn
      cycles = Control::jp_cond(state, !GET_FLAG(P_FLAG, state.registers),
                                state.getNextWordFromPC());
      break;

    case 0xEA: // JP PE, nn
      cycles = Control::jp_cond(state, GET_FLAG(P_FLAG, state.registers),
                                state.getNextWordFromPC());
      break;

    case 0xF2: // JP P, nn
      cycles = Control::jp_cond(state, !GET_FLAG(S_FLAG, state.registers),
                                state.getNextWordFromPC());
      break;

    case 0xFA: // JP M, nn
      cycles = Control::jp_cond(state, GET_FLAG(S_FLAG, state.registers),
                                state.getNextWordFromPC());
      break;

    // Conditional Returns
    case 0xC0: // RET NZ
      cycles = Control::ret_cond(state, !GET_FLAG(Z_FLAG, state.registers));
      break;

    case 0xC8: // RET Z
      cycles = Control::ret_cond(state, GET_FLAG(Z_FLAG, state.registers));
      break;

    case 0xD0: // RET NC
      cycles = Control::ret_cond(state, !GET_FLAG(C_FLAG, state.registers));
      break;

    case 0xD8: // RET C
      cycles = Control::ret_cond(state, GET_FLAG(C_FLAG, state.registers));
      break;

    case 0xE0: // RET PO
      cycles = Control::ret_cond(state, !GET_FLAG(P_FLAG, state.registers));
      break;

    case 0xE8: // RET PE
      cycles = Control::ret_cond(state, GET_FLAG(P_FLAG, state.registers));
      break;

    case 0xF0: // RET P
      cycles = Control::ret_cond(state, !GET_FLAG(S_FLAG, state.registers));
      break;

    case 0xF8: // RET M
      cycles = Control::ret_cond(state, GET_FLAG(S_FLAG, state.registers));
      break;

    case 0xC9: // RET
      cycles = Control::ret(state);
      break;

    case 0xCD: // CALL nn
      cycles = Control::call(state, state.getNextWordFromPC());
      break;

    // Conditional CALLs
    case 0xC4: // CALL NZ, nn
      cycles = Control::call_cond(state, !GET_FLAG(Z_FLAG, state.registers),
                                  state.getNextWordFromPC());
      break;

    case 0xCB: // Prefix CB
      cycles = exec_cb_opcode();
      break;

    case 0xED: // Extended
      cycles = exec_ed_opcode();
      break;

    case 0xCC: // CALL Z, nn
      cycles = Control::call_cond(state, GET_FLAG(Z_FLAG, state.registers),
                                  state.getNextWordFromPC());
      break;

    case 0xD4: // CALL NC, nn
      cycles = Control::call_cond(state, !GET_FLAG(C_FLAG, state.registers),
                                  state.getNextWordFromPC());
      break;

    case 0xDB: { // IN A, (n)
      byte port = state.getNextByteFromPC();
      state.registers.PC++;
      cycles = IO::in_a_n(state, port);
      break;
    }

    case 0xDC: // CALL C, nn
      cycles = Control::call_cond(state, GET_FLAG(C_FLAG, state.registers),
                                  state.getNextWordFromPC());
      break;

    case 0xE4: // CALL PO, nn
      cycles = Control::call_cond(state, !GET_FLAG(P_FLAG, state.registers),
                                  state.getNextWordFromPC());
      break;

    case 0xEC: // CALL PE, nn
      cycles = Control::call_cond(state, GET_FLAG(P_FLAG, state.registers),
                                  state.getNextWordFromPC());
      break;

    case 0xF4: // CALL P, nn
      cycles = Control::call_cond(state, !GET_FLAG(S_FLAG, state.registers),
                                  state.getNextWordFromPC());
      break;

    case 0xFC: // CALL M, nn
      cycles = Control::call_cond(state, GET_FLAG(S_FLAG, state.registers),
                                  state.getNextWordFromPC());
      break;

    case 0xE3: // EX (SP), HL
      Load::ex_sp_hl(state);
      cycles = 19;
      break;

    case 0xEB: // EX DE, HL
      Load::ex_de_hl(state);
      cycles = 4;
      break;

    case 0xF9: // LD SP, HL
      Load::ld_sp_hl(state);
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
      Arithmetic::add8(state, state.registers.B);
      cycles = 4;
      break;
    case 0x81:
      Arithmetic::add8(state, state.registers.C);
      cycles = 4;
      break;
    case 0x82:
      Arithmetic::add8(state, state.registers.D);
      cycles = 4;
      break;
    case 0x83:
      Arithmetic::add8(state, state.registers.E);
      cycles = 4;
      break;
    case 0x84:
      Arithmetic::add8(state, state.registers.H);
      cycles = 4;
      break;
    case 0x85:
      Arithmetic::add8(state, state.registers.L);
      cycles = 4;
      break;
    case 0x86:
      Arithmetic::add8(state, m_memory[state.registers.HL]);
      cycles = 7;
      break;
    case 0x87:
      Arithmetic::add8(state, state.registers.A);
      cycles = 4;
      break;

    // ADC A, r (0x88-0x8F)
    case 0x88:
      Arithmetic::adc8(state, state.registers.B);
      cycles = 4;
      break;
    case 0x89:
      Arithmetic::adc8(state, state.registers.C);
      cycles = 4;
      break;
    case 0x8A:
      Arithmetic::adc8(state, state.registers.D);
      cycles = 4;
      break;
    case 0x8B:
      Arithmetic::adc8(state, state.registers.E);
      cycles = 4;
      break;
    case 0x8C:
      Arithmetic::adc8(state, state.registers.H);
      cycles = 4;
      break;
    case 0x8D:
      Arithmetic::adc8(state, state.registers.L);
      cycles = 4;
      break;
    case 0x8E:
      Arithmetic::adc8(state, m_memory[state.registers.HL]);
      cycles = 7;
      break;
    case 0x8F:
      Arithmetic::adc8(state, state.registers.A);
      cycles = 4;
      break;

    // SUB r (0x90-0x97)
    case 0x90:
      Arithmetic::sub8(state, state.registers.B);
      cycles = 4;
      break;
    case 0x91:
      Arithmetic::sub8(state, state.registers.C);
      cycles = 4;
      break;
    case 0x92:
      Arithmetic::sub8(state, state.registers.D);
      cycles = 4;
      break;
    case 0x93:
      Arithmetic::sub8(state, state.registers.E);
      cycles = 4;
      break;
    case 0x94:
      Arithmetic::sub8(state, state.registers.H);
      cycles = 4;
      break;
    case 0x95:
      Arithmetic::sub8(state, state.registers.L);
      cycles = 4;
      break;
    case 0x96:
      Arithmetic::sub8(state, m_memory[state.registers.HL]);
      cycles = 7;
      break;
    case 0x97:
      Arithmetic::sub8(state, state.registers.A);
      cycles = 4;
      break;

    // SBC A, r (0x98-0x9F)
    case 0x98:
      Arithmetic::sbc8(state, state.registers.B);
      cycles = 4;
      break;
    case 0x99:
      Arithmetic::sbc8(state, state.registers.C);
      cycles = 4;
      break;
    case 0x9A:
      Arithmetic::sbc8(state, state.registers.D);
      cycles = 4;
      break;
    case 0x9B:
      Arithmetic::sbc8(state, state.registers.E);
      cycles = 4;
      break;
    case 0x9C:
      Arithmetic::sbc8(state, state.registers.H);
      cycles = 4;
      break;
    case 0x9D:
      Arithmetic::sbc8(state, state.registers.L);
      cycles = 4;
      break;
    case 0x9E:
      Arithmetic::sbc8(state, m_memory[state.registers.HL]);
      cycles = 7;
      break;
    case 0x9F:
      Arithmetic::sbc8(state, state.registers.A);
      cycles = 4;
      break;

    // AND r (0xA0-0xA7)
    case 0xA0:
      Logic::and8(state, state.registers.B);
      cycles = 4;
      break;
    case 0xA1:
      Logic::and8(state, state.registers.C);
      cycles = 4;
      break;
    case 0xA2:
      Logic::and8(state, state.registers.D);
      cycles = 4;
      break;
    case 0xA3:
      Logic::and8(state, state.registers.E);
      cycles = 4;
      break;
    case 0xA4:
      Logic::and8(state, state.registers.H);
      cycles = 4;
      break;
    case 0xA5:
      Logic::and8(state, state.registers.L);
      cycles = 4;
      break;
    case 0xA6:
      Logic::and8(state, m_memory[state.registers.HL]);
      cycles = 7;
      break;
    case 0xA7:
      Logic::and8(state, state.registers.A);
      cycles = 4;
      break;

    // XOR r (0xA8-0xAF)
    case 0xA8:
      Logic::xor8(state, state.registers.B);
      cycles = 4;
      break;
    case 0xA9:
      Logic::xor8(state, state.registers.C);
      cycles = 4;
      break;
    case 0xAA:
      Logic::xor8(state, state.registers.D);
      cycles = 4;
      break;
    case 0xAB:
      Logic::xor8(state, state.registers.E);
      cycles = 4;
      break;
    case 0xAC:
      Logic::xor8(state, state.registers.H);
      cycles = 4;
      break;
    case 0xAD:
      Logic::xor8(state, state.registers.L);
      cycles = 4;
      break;
    case 0xAE:
      Logic::xor8(state, m_memory[state.registers.HL]);
      cycles = 7;
      break;
    case 0xAF:
      Logic::xor8(state, state.registers.A);
      cycles = 4;
      break;

    // OR r (0xB0-0xB7)
    case 0xB0:
      Logic::or8(state, state.registers.B);
      cycles = 4;
      break;
    case 0xB1:
      Logic::or8(state, state.registers.C);
      cycles = 4;
      break;
    case 0xB2:
      Logic::or8(state, state.registers.D);
      cycles = 4;
      break;
    case 0xB3:
      Logic::or8(state, state.registers.E);
      cycles = 4;
      break;
    case 0xB4:
      Logic::or8(state, state.registers.H);
      cycles = 4;
      break;
    case 0xB5:
      Logic::or8(state, state.registers.L);
      cycles = 4;
      break;
    case 0xB6:
      Logic::or8(state, m_memory[state.registers.HL]);
      cycles = 7;
      break;
    case 0xB7:
      Logic::or8(state, state.registers.A);
      cycles = 4;
      break;

    // CP r (0xB8-0xBF)
    case 0xB8:
      Arithmetic::cp8(state, state.registers.B);
      cycles = 4;
      break;
    case 0xB9:
      Arithmetic::cp8(state, state.registers.C);
      cycles = 4;
      break;
    case 0xBA:
      Arithmetic::cp8(state, state.registers.D);
      cycles = 4;
      break;
    case 0xBB:
      Arithmetic::cp8(state, state.registers.E);
      cycles = 4;
      break;
    case 0xBC:
      Arithmetic::cp8(state, state.registers.H);
      cycles = 4;
      break;
    case 0xBD:
      Arithmetic::cp8(state, state.registers.L);
      cycles = 4;
      break;
    case 0xBE:
      Arithmetic::cp8(state, m_memory[state.registers.HL]);
      cycles = 7;
      break;
    case 0xBF:
      Arithmetic::cp8(state, state.registers.A);
      cycles = 4;
      break;

    // ------------------------------------------------------------------------
    // Immediate Arithmetic (0xC6, 0xCE, 0xD6, 0xDE, 0xE6, 0xEE, 0xF6, 0xFE)
    // ------------------------------------------------------------------------
    case 0xC6: { // ADD A, n
      byte n = m_memory[state.registers.PC++];
      Arithmetic::add8(state, n);
      cycles = 7;
      break;
    }
    case 0xCE: { // ADC A, n
      byte n = m_memory[state.registers.PC++];
      Arithmetic::adc8(state, n);
      cycles = 7;
      break;
    }
    case 0xD6: { // SUB n
      byte n = m_memory[state.registers.PC++];
      Arithmetic::sub8(state, n);
      cycles = 7;
      break;
    }
    case 0xDE: { // SBC A, n
      byte n = m_memory[state.registers.PC++];
      Arithmetic::sbc8(state, n);
      cycles = 7;
      break;
    }
    case 0xE6: { // AND n
      byte n = m_memory[state.registers.PC++];
      Logic::and8(state, n);
      cycles = 7;
      break;
    }
    case 0xEE: { // XOR n
      byte n = m_memory[state.registers.PC++];
      Logic::xor8(state, n);
      cycles = 7;
      break;
    }
    case 0xF6: { // OR n
      byte n = m_memory[state.registers.PC++];
      Logic::or8(state, n);
      cycles = 7;
      break;
    }
    case 0xFE: { // CP n
      byte n = m_memory[state.registers.PC++];
      Arithmetic::cp8(state, n);
      cycles = 7;
      break;
    }

    // ------------------------------------------------------------------------
    // INC/DEC 8-bit
    // ------------------------------------------------------------------------
    case 0x04:
      Arithmetic::inc8(state, state.registers.B);
      cycles = 4;
      break;
    case 0x05:
      Arithmetic::dec8(state, state.registers.B);
      cycles = 4;
      break;
    case 0x0C:
      Arithmetic::inc8(state, state.registers.C);
      cycles = 4;
      break;
    case 0x0D:
      Arithmetic::dec8(state, state.registers.C);
      cycles = 4;
      break;
    case 0x14:
      Arithmetic::inc8(state, state.registers.D);
      cycles = 4;
      break;
    case 0x15:
      Arithmetic::dec8(state, state.registers.D);
      cycles = 4;
      break;
    case 0x1C:
      Arithmetic::inc8(state, state.registers.E);
      cycles = 4;
      break;
    case 0x1D:
      Arithmetic::dec8(state, state.registers.E);
      cycles = 4;
      break;
    case 0x24:
      Arithmetic::inc8(state, state.registers.H);
      cycles = 4;
      break;
    case 0x25:
      Arithmetic::dec8(state, state.registers.H);
      cycles = 4;
      break;
    case 0x2C:
      Arithmetic::inc8(state, state.registers.L);
      cycles = 4;
      break;
    case 0x2D:
      Arithmetic::dec8(state, state.registers.L);
      cycles = 4;
      break;

    case 0x3C:
      Arithmetic::inc8(state, state.registers.A);
      cycles = 4;
      break;
    case 0x3D:
      Arithmetic::dec8(state, state.registers.A);
      cycles = 4;
      break;

    case 0x34: { // INC (HL)
      byte val = m_memory[state.registers.HL];
      Arithmetic::inc8(state, val);
      writeMem(state.registers.HL, val);
      cycles = 11;
      break;
    }
    case 0x35: { // DEC (HL)
      byte val = m_memory[state.registers.HL];
      Arithmetic::dec8(state, val);
      writeMem(state.registers.HL, val);
      cycles = 11;
      break;
    }

      // ------------------------------------------------------------------------
      // Misc
      // ------------------------------------------------------------------------

    case 0x3F: // CCF
      Logic::ccf(state);
      cycles = 4;
      break;

    case 0x27: // DAA
      Arithmetic::daa(state);
      cycles = 4;
      break;

    case 0x2F: // CPL
      Logic::cpl(state);
      cycles = 4;
      break;

    case 0x37: // SCF
      Logic::scf(state);
      cycles = 4;
      break;

    case 0xF3: // DI
      cycles = Control::di(state);
      break;

    case 0xFB: // EI
      cycles = Control::ei(state);
      break;

    // ------------------------------------------------------------------------
    // INC/DEC 16-bit (BC, DE, HL, SP)
    // ------------------------------------------------------------------------
    case 0x03:
      Arithmetic::inc16(state, state.registers.BC);
      cycles = 6;
      break;
    case 0x0B:
      Arithmetic::dec16(state, state.registers.BC);
      cycles = 6;
      break;
    case 0x13:
      Arithmetic::inc16(state, state.registers.DE);
      cycles = 6;
      break;
    case 0x1B:
      Arithmetic::dec16(state, state.registers.DE);
      cycles = 6;
      break;
    case 0x23:
      Arithmetic::inc16(state, state.registers.HL);
      cycles = 6;
      break;
    case 0x2B:
      Arithmetic::dec16(state, state.registers.HL);
      cycles = 6;
      break;
    case 0x33:
      Arithmetic::inc16(state, state.registers.SP);
      cycles = 6;
      break;
    case 0x3B:
      Arithmetic::dec16(state, state.registers.SP);
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
      // Fallback removed (Legacy OpCode classes removed)
      // Any unhandled opcode acts as NOP or Error
      /*
      state.registers.PC--; // Rewind PC for catalogue lookup
      OpCode *opCode = getNextInstruction();
      if (opCode != nullptr) { ... }
      */

      byte unknownOpcode = state.memory[state.registers.PC];
      char errorMsg[100];
      snprintf(errorMsg, sizeof(errorMsg), "Unknown opcode %02X at address %d",
               unknownOpcode, this->state.registers.PC);
      lastError = std::string(errorMsg);

      debug("Unknown opcode %02X at address %d\n", unknownOpcode,
            this->state.registers.PC);
      running = false;
    }
  }
  // } // Extraneous brace removed
  audio.flush();

  // Audio Sync: Throttle execution to match audio consumption rate
  // If buffer has > 3 frames of audio (approx 60ms), slow down.
  // This locks emulation speed to the audio card clock (44.1kHz).
  if (!turbo) {
    while (audio.getBufferSize() > 2646) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
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
        // Only play tape if there are blocks remaining to load
        // (fast load may have already loaded everything)
        if (!state.tape.isFinished()) {
          state.tape.play();
        }
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

// Stack helpers removed (moved to LoadInstructions.h)

/**
 * Read the next instruction and process it
 * @return
 */
// OpCode *Processor::getNextInstruction() { ... } // Removed
// ============================================================================
// Opcode Helper Methods
// ============================================================================

// Helper methods removed (moved to Arithmetic/LogicInstructions.h)

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
    Bit::rrd(state);
    cycles = 18;
    break;
  case 0x6F:
    Bit::rld(state);
    cycles = 18;
    break;

  // RETI
  case 0x4D:
    cycles = Control::reti(state);
    break;

  // RETN
  case 0x45:
    cycles = Control::retn(state);
    break;
  // IN r, (C)
  case 0x40:
    IO::in_r_c(state, state.registers.B);
    cycles = 12;
    break;
  case 0x48:
    IO::in_r_c(state, state.registers.C);
    cycles = 12;
    break;
  case 0x50:
    IO::in_r_c(state, state.registers.D);
    cycles = 12;
    break;
  case 0x58:
    IO::in_r_c(state, state.registers.E);
    cycles = 12;
    break;
  case 0x60:
    IO::in_r_c(state, state.registers.H);
    cycles = 12;
    break;
  case 0x68:
    IO::in_r_c(state, state.registers.L);
    cycles = 12;
    break;
  case 0x78:
    IO::in_r_c(state, state.registers.A);
    cycles = 12;
    break;

  // SBC HL, rr
  case 0x42:
    Arithmetic::sbc16(state, state.registers.HL, state.registers.BC);
    cycles = 15;
    break;
  case 0x52:
    Arithmetic::sbc16(state, state.registers.HL, state.registers.DE);
    cycles = 15;
    break;
  case 0x62:
    Arithmetic::sbc16(state, state.registers.HL, state.registers.HL);
    cycles = 15;
    break;
  case 0x72:
    Arithmetic::sbc16(state, state.registers.HL, state.registers.SP);
    cycles = 15;
    break;

  // ADC HL, rr
  case 0x4A:
    Arithmetic::adc16(state, state.registers.HL, state.registers.BC);
    cycles = 15;
    break;
  case 0x5A:
    Arithmetic::adc16(state, state.registers.HL, state.registers.DE);
    cycles = 15;
    break;
  case 0x6A:
    Arithmetic::adc16(state, state.registers.HL, state.registers.HL);
    cycles = 15;
    break;
  case 0x7A:
    Arithmetic::adc16(state, state.registers.HL, state.registers.SP);
    cycles = 15;
    break;

  // LD (nn), rr - Consumes 2 byte operand (nn)
  case 0x43:
    Load::ld_nn_rr(state, state.getNextWordFromPC(), state.registers.BC);
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x53:
    Load::ld_nn_rr(state, state.getNextWordFromPC(), state.registers.DE);
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x63:
    Load::ld_nn_rr(state, state.getNextWordFromPC(), state.registers.HL);
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x73:
    Load::ld_nn_rr(state, state.getNextWordFromPC(), state.registers.SP);
    state.registers.PC += 2;
    cycles = 20;
    break;

  // LD rr, (nn) - Consumes 2 byte operand (nn)
  case 0x4B:
    Load::ld_rr_nn(state, state.registers.BC, state.getNextWordFromPC());
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x5B:
    Load::ld_rr_nn(state, state.registers.DE, state.getNextWordFromPC());
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x6B:
    Load::ld_rr_nn(state, state.registers.HL, state.getNextWordFromPC());
    state.registers.PC += 2;
    cycles = 20;
    break;
  case 0x7B:
    Load::ld_rr_nn(state, state.registers.SP, state.getNextWordFromPC());
    state.registers.PC += 2;
    cycles = 20;
    break;

  case 0x44: // NEG
    Arithmetic::neg8(state);
    cycles = 8;
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
  case 0xA0:
    cycles = Load::ldi(state);
    break;
  case 0xA8:
    cycles = Load::ldd(state);
    break;
  case 0xB0:
    cycles = Load::ldir(state);
    break;
  case 0xB8:
    cycles = Load::lddr(state);
    break;
  case 0xA1:
    cycles = Control::cpi(state);
    break;
  case 0xA9:
    cycles = Control::cpd(state);
    break;
  case 0xB1:
    cycles = Control::cpir(state);
    break;
  case 0xB9:
    cycles = Control::cpdr(state);
    break;

  // Block I/O
  case 0xA2:
    cycles = cycles = IO::ini(state);
    break;
  case 0xB2:
    cycles = cycles = IO::inir(state);
    break;
  case 0xAA:
    cycles = cycles = IO::ind(state);
    break;
  case 0xBA:
    cycles = cycles = IO::indr(state);
    break;
  case 0xA3:
    cycles = cycles = IO::outi(state);
    break;
  case 0xB3:
    cycles = cycles = IO::otir(state);
    break;
  case 0xAB:
    cycles = cycles = IO::outd(state);
    break;
  case 0xBB:
    cycles = cycles = IO::otdr(state);
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
      Bit::rlc(state, *regPtr);
      break;
    case 1:
      Bit::rrc(state, *regPtr);
      break;
    case 2:
      Bit::rl(state, *regPtr);
      break;
    case 3:
      Bit::rr(state, *regPtr);
      break;
    case 4:
      Bit::sla(state, *regPtr);
      break;
    case 5:
      Bit::sra(state, *regPtr);
      break;
    case 6:
      Bit::sll(state, *regPtr);
      break; // SLL (undocumented)
    case 7:
      Bit::srl(state, *regPtr);
      break;
    }
  } else if (x == 1) { // BIT
    if (isMem) {
      // Z80 Undocumented: BIT n, (HL) sets X/Y from internal register (MEMPTR
      // high byte), which is H (High byte of HL) Here, MEMPTR = HL. So pass
      // (hlAddr >> 8) as the source for X/Y flags, but 'val' for Z flag test.
      // We need to modify Bit::bit signature or handle it here.
      // Let's modify Bit::bit to take an optional 'undocSource'
      // OR: handle flags manually here.
      Bit::bitMem(state, y, *regPtr, (hlAddr >> 8));
      return cycles;
    } else {
      Bit::bit(state, y, *regPtr);
    }
  } else if (x == 2) { // RES
    Bit::res(state, y, *regPtr);
  } else if (x == 3) { // SET
    Bit::set(state, y, *regPtr);
  }

  // Write back if memory and NOT BIT (BIT doesn't modify)
  if (isMem && x != 1) {
    writeMem(hlAddr, *regPtr);
  }

  return cycles;
}
int Processor::exec_index_opcode(byte prefix) {
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
    // specified by 'z' (if not 6).
    // We MUST implement this for game compatibility (e.g. Jetpac).

    int cycles = 23; // Usually 23 for indexed bit ops

    // Lambda for undocumented writeback
    auto setUndocReg = [&](int r, byte v) {
      switch (r) {
      case 0:
        state.registers.B = v;
        break;
      case 1:
        state.registers.C = v;
        break;
      case 2:
        state.registers.D = v;
        break;
      case 3:
        state.registers.E = v;
        break;
      case 4:
        state.registers.H = v;
        break;
      case 5:
        state.registers.L = v;
        break;
      case 7:
        state.registers.A = v;
        break;
      }
    };

    if (x == 0) { // Rotate/Shift
      switch (y) {
      case 0:
        Bit::rlc(state, val);
        break;
      case 1:
        Bit::rrc(state, val);
        break;
      case 2:
        Bit::rl(state, val);
        break;
      case 3:
        Bit::rr(state, val);
        break;
      case 4:
        Bit::sla(state, val);
        break;
      case 5:
        Bit::sra(state, val);
        break;
      case 6:
        Bit::sll(state, val);
        break;
      case 7:
        Bit::srl(state, val);
        break;
      }
      writeMem(addr, val);
      // Undocumented register writeback
      if (z != 6)
        setUndocReg(z, val);
    } else if (x == 1) { // BIT
      // Z80 Undocumented: BIT n, (IX+d).
      // X/Y flags taken from High Byte of (IX+d) address.
      // 'addr' is the calculated effective address.
      Bit::bitMem(state, y, val, (addr >> 8));
      cycles = 20;       // BIT is 20
                         // No writeback to memory OR register
    } else if (x == 2) { // RES
      Bit::res(state, y, val);
      writeMem(addr, val);
      // Undocumented register writeback
      if (z != 6)
        setUndocReg(z, val);
    } else if (x == 3) { // SET
      Bit::set(state, y, val);
      writeMem(addr, val);
      // Undocumented register writeback
      if (z != 6)
        setUndocReg(z, val);
    }

    return cycles;
  }

  // Standard Index Opcodes
  // Helper for High/Low byte access (Little Endian host assumed)
  byte *idxL = (byte *)&idx;
  byte *idxH = (byte *)&idx + 1;

  int cycles = 0;

  switch (opcode) {
  // 09, 19, 29, 39: ADD IX, rr
  case 0x09:
    Arithmetic::add16(state, idx, state.registers.BC);
    cycles = 15;
    break;
  case 0x19:
    Arithmetic::add16(state, idx, state.registers.DE);
    cycles = 15;
    break;
  case 0x29:
    Arithmetic::add16(state, idx, idx);
    cycles = 15;
    break;
  case 0x39:
    Arithmetic::add16(state, idx, state.registers.SP);
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
    Arithmetic::inc8(state, *idxH);
    cycles = 8;
    break;
  }
  // 25: DEC IXH
  case 0x25: {
    Arithmetic::dec8(state, *idxH);
    cycles = 8;
    break;
  }
  // 2C: INC IXL
  case 0x2C: {
    Arithmetic::inc8(state, *idxL);
    cycles = 8;
    break;
  }
  // 2D: DEC IXL
  case 0x2D: {
    Arithmetic::dec8(state, *idxL);
    cycles = 8;
    break;
  }
  // 26: LD IXH, n
  case 0x26: {
    byte n = state.getNextByteFromPC();
    state.registers.PC++;
    *idxH = n;
    cycles = 11;
    break;
  }
  // 2E: LD IXL, n
  case 0x2E: {
    byte n = state.getNextByteFromPC();
    state.registers.PC++;
    *idxL = n;
    cycles = 11;
    break;
  }

  // 34: INC (IX+d)
  case 0x34: {
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    word addr = idx + (int8_t)d;
    byte val = state.memory[addr];
    // Use helper for correct flags (H, P/V)
    Arithmetic::inc8(state, val);
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
    Arithmetic::dec8(state, val);
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

  // Undocumented: Arithmetic with IXH/IXL
  case 0x84: // ADD A, IXH
    Arithmetic::add8(state, *idxH);
    cycles = 8;
    break;
  case 0x85: // ADD A, IXL
    Arithmetic::add8(state, *idxL);
    cycles = 8;
    break;
  case 0x8C: // ADC A, IXH
    Arithmetic::adc8(state, *idxH);
    cycles = 8;
    break;
  case 0x8D: // ADC A, IXL
    Arithmetic::adc8(state, *idxL);
    cycles = 8;
    break;
  case 0x94: // SUB IXH
    Arithmetic::sub8(state, *idxH);
    cycles = 8;
    break;
  case 0x95: // SUB IXL
    Arithmetic::sub8(state, *idxL);
    cycles = 8;
    break;
  case 0x9C: // SBC A, IXH
    Arithmetic::sbc8(state, *idxH);
    cycles = 8;
    break;
  case 0x9D: // SBC A, IXL
    Arithmetic::sbc8(state, *idxL);
    cycles = 8;
    break;
  case 0xA4: // AND IXH
    Logic::and8(state, *idxH);
    cycles = 8;
    break;
  case 0xA5: // AND IXL
    Logic::and8(state, *idxL);
    cycles = 8;
    break;
  case 0xAC: // XOR IXH
    Logic::xor8(state, *idxH);
    cycles = 8;
    break;
  case 0xAD: // XOR IXL
    Logic::xor8(state, *idxL);
    cycles = 8;
    break;
  // B4, B5: OR - already implemented above
  case 0xBC: // CP IXH
    Arithmetic::cp8(state, *idxH);
    cycles = 8;
    break;
  case 0xBD: // CP IXL
    Arithmetic::cp8(state, *idxL);
    cycles = 8;
    break;

  // ALU (IX+d)
  case 0x86: { // ADD A, (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    Arithmetic::add8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0x8E: { // ADC A, (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    Arithmetic::adc8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0x96: { // SUB (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    Arithmetic::sub8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0x9E: { // SBC A, (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    Arithmetic::sbc8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0xA6: { // AND (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    Logic::and8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0xAE: { // XOR (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    Logic::xor8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0xB6: { // OR (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    Logic::or8(state, state.memory[idx + (int8_t)d]);
    cycles = 19;
    break;
  }
  case 0xBE: { // CP (IX+d)
    byte d = state.getNextByteFromPC();
    state.registers.PC++;
    Arithmetic::cp8(state, state.memory[idx + (int8_t)d]);
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
    Logic::or8(state, *idxH);
    cycles = 8;
    break;
  }
  // B5: OR IXL (Requested)
  case 0xB5: {
    Logic::or8(state, *idxL);
    cycles = 8;
    break;
  }

  case 0xE1: {
    idx = Load::pop16(state);
    cycles = 14;
    break;
  }

  // 99: SBC A, C (Prefix ignored)
  case 0x99: {
    Arithmetic::sbc8(state, state.registers.C);
    cycles = 4;
    break;
  }

  // E5: PUSH IX
  case 0xE5: {
    Load::push16(state, idx);
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

  // I/O Instructions - IX/IY prefix is ignored
  case 0xD3: { // OUT (n), A
    byte port = state.getNextByteFromPC();
    state.registers.PC++;
    cycles = IO::out_n_a(state, port);
    break;
  }
  case 0xDB: { // IN A, (n)
    byte port = state.getNextByteFromPC();
    state.registers.PC++;
    cycles = IO::in_a_n(state, port);
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

  return cycles;
}
