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
#include "RotateOpcodes.h"
#include "../../../utils/debug.h"
#include "../../ProcessorState.h"

RotateOpcodes::RotateOpcodes() : OpCodeProvider() {

  // RRCA (0x0F) - Rotate A right with branch carry
  // C = A0, A7 = A0, A = A >> 1
  // Flags: H, N reset. C from A0. Z, S, P/V not affected.
  createOpCode(0x0F, "RRCA", [](ProcessorState &state) {
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

    // Z, S, P/V not affected
    return 4;
  });

  // RLCA (0x07) - Rotate A left with branch carry
  // C = A7, A0 = A7, A = A << 1
  createOpCode(0x07, "RLCA", [](ProcessorState &state) {
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
    return 4;
  });

  // RRA (0x1F) - Rotate A right through carry
  // Old C -> A7, A0 -> C
  createOpCode(0x1F, "RRA", [](ProcessorState &state) {
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
    return 4;
  });

  // RLA (0x17) - Rotate A left through carry
  // Old C -> A0, A7 -> C
  createOpCode(0x17, "RLA", [](ProcessorState &state) {
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
    return 4;
  });
}
