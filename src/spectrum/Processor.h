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

#ifndef ZXEMULATOR_PROCESSOR_H
#define ZXEMULATOR_PROCESSOR_H

#include "Opcodes/OpCodeCatalogue.h"

#include "Audio.h"

class Processor {
  friend class InstructionTest;

private:
  // State variables
  ProcessorState state;
  Audio audio;
  OpCodeCatalogue catalogue = OpCodeCatalogue();

  bool running = false;
  bool paused = false;
  bool stepRequest = false;

  // Auto-Load
  bool autoLoadTape = false;
  long frameCounter = 0;
  int autoLoadStep = 0;
  int keyHoldFrames = 0;

  // Internal methods
  OpCode *getNextInstruction();

  // Fast direct memory access
  byte *m_memory = nullptr;

  // Write with ROM protection
  void writeMem(word address, byte value);
  // Stack helpers with safe memory access
  void push16(word value);
  word pop16();

  // Helper methods for instruction groups
  void op_load(byte opcode);
  void op_arithmetic(byte opcode);
  void op_logic(byte opcode);
  void op_rotate_shift(byte opcode);
  void op_bit(byte opcode);
  void op_jump(byte opcode);
  void op_stack(byte opcode);
  void op_io(byte opcode);
  void op_misc(byte opcode);

  // Extended instruction handlers
  int exec_ed_opcode();
  int exec_cb_opcode();
  void exec_index_opcode(byte prefix); // DD or FD

  // ALU Helpers
  void add8(byte val); // ADD A, val
  void adc8(byte val); // ADC A, val
  void sub8(byte val); // SUB val
  void sbc8(byte val); // SBC A, val
  void and8(byte val); // AND val
  void xor8(byte val); // XOR val
  void or8(byte val);  // OR val
  void cp8(byte val);  // CP val
  void inc8(byte &reg);
  void dec8(byte &reg);

  // 16-bit ALU Helpers for convenience (optional, but good for consistency)
  void add16(word &dest, word src);
  void adc16(word &dest, word src);
  void sbc16(word &dest, word src);
  void inc16(word &reg);
  void dec16(word &reg);

public:
  explicit Processor();

  OpCode *getOpCode(byte b) { return catalogue.lookupOpcode(b); }

  void init(const char *romFile);
  void loadTape(Tape tape);
  void loadSnapshot(const char *filename);

  void run();
  void executeFrame();

  std::string lastError = "";

  std::vector<byte> fetchOperands(int count);

  // Bit Manipulation Helpers
  void rlc(byte &val);
  void rrc(byte &val);
  void rl(byte &val);
  void rr(byte &val);
  void sla(byte &val);
  void sra(byte &val);
  void sll(byte &val);
  void srl(byte &val);
  void bit(int bit, byte val);
  void set(int bit, byte &val);
  void res(int bit, byte &val);

  // Extended (ED) Helpers
  void op_ed_ld_nn_rr(word nn, word rr);  // LD (nn), rr
  void op_ed_ld_rr_nn(word &rr, word nn); // LD rr, (nn)
  void op_ed_in_r_C(byte &r);             // IN r, (C)
  void op_ed_out_C_r(byte r);             // OUT (C), r
  void op_ed_sbc16(word &dest, word src);
  void op_ed_adc16(word &dest, word src);

  // RRD/RLD
  void op_ed_rrd();
  void op_ed_rld();

  // Block Instructions
  int op_ed_ldir();
  int op_ed_lddr();
  int op_ed_cpir();
  int op_ed_cpdr();
  int op_ed_ini();
  int op_ed_ind();
  int op_ed_outi();
  int op_ed_outd();

  VideoBuffer *getVideoBuffer();
  ProcessorState &getState() { return state; } // Expose for debugger
  bool isRunning() const { return running; }
  const std::string &getLastError() const { return lastError; }

  void shutdown();

  // Debug control
  void reset();
  void pause() { paused = true; }
  void resume() { paused = false; }
  void step() {
    if (paused)
      stepRequest = true;
  }
  bool isPaused() const { return paused; }
};

#endif // ZXEMULATOR_PROCESSOR_H
