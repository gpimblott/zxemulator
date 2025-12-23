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

#include "IOOpcodes.h"

IOOpcodes::IOOpcodes() : OpCodeProvider() {
  createOpCode(OUT, "OUT", processOUT);
  createOpCode(IN_A_N, "IN A, (n)", processIN_A_N);
}

int IOOpcodes::processOUT(ProcessorState &state) {
  byte port = state.getNextByteFromPC(); // Port address from n
  state.incPC();
  byte value = state.registers.A; // Value to write is in A
  // debug("OUT (%02X), A", port);

  // Port FE (or any even port on 48K) controls border color and speaker
  // Bit 0-2: Border color (0-7)
  // Bit 3: MIC output
  // Bit 4: Speaker (beeper)
  if ((port & 0x01) == 0) {
    byte borderColor = state.registers.A & 0x07;
    state.memory.getVideoBuffer()->setBorderColor(borderColor,
                                                  state.getFrameTStates());

    // Bit 4: Speaker, Bit 3: MIC
    state.setSpeakerBit((value & 0x10) != 0);
    state.setMicBit((value & 0x08) != 0);
  }

  return 11;
}

int IOOpcodes::processIN_A_N(ProcessorState &state) {
  byte port = state.getNextByteFromPC();
  state.incPC();
  // debug("IN A, (%02X)", port);

  // Keyboard/Ear Reading
  // Port address: In Z80, I/O IN A,(n) places A on the high half of the address
  // bus and n on the low half. The keyboard is read by checking the high byte
  // (A).
  byte highByte = state.registers.A;

  // Actually IN A, (n) reads port (A << 8) | n
  // But for the ULA, we primarily care about the high byte for keyboard rows
  // (FE, FD, etc) And the low byte usually is FE for ULA.

  // Technically we should check if the low byte (port) is FE (254).
  // But the Spectrum ULA responds to any even port number.
  if ((port & 0x01) == 0) {
    byte ear = state.tape.getEarBit() ? 0x40 : 0x00; // EAR is bit 6
    state.registers.A = state.keyboard.readPort(highByte) | ear;
  } else if ((port & 0x1F) == 0x1F) {
    // Kempston Joystick (Port 31)
    state.registers.A = state.keyboard.readKempstonPort();
  } else {
    // Floating bus or other devices?
    state.registers.A = 0xFF;
  }

  return 11;
}
