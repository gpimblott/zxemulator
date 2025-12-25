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

    // RESTORED CASES (Accidentally deleted)
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

    // Stack Operations (PUSH/POP)
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

    default:
      // Handle Grouped 8-bit Loads (0x40 - 0x7F)
      // Note: 0x76 (HALT) is handled by specific case above, so it wont reach
      // here.
      if (opcode >= 0x40 && opcode <= 0x7F) {
        cycles = exec_loads_8bit(opcode);
        handled = true;
        break;
      }
      // Handle Grouped 8-bit ALU (0x80 - 0xBF)
      if (opcode >= 0x80 && opcode <= 0xBF) {
        cycles = exec_alu_8bit(opcode);
        handled = true;
        break;
      }

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
