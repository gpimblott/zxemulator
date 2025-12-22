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

#ifndef ZXEMULATOR_KEYBOARD_H
#define ZXEMULATOR_KEYBOARD_H

#include "../utils/BaseTypes.h"
#include <vector>

class Keyboard {
private:
  // 8 rows (lines), 5 bits each.
  // Index 0 = 0xFE (SHIFT ... V)
  // Index 1 = 0xFD (A ... G)
  // ...
  // Store as bytes where 0 = pressed, 1 = released?
  // Usually it's easier to store 1=pressed internally and invert on read.
  // Let's store 1 = pressed for simplicity, return ~(pressed) masked.
  emulator_types::byte keyLines[8];

public:
  Keyboard();

  /**
   * Set the state of a specific key
   * @param line 0-7 (corresponding to high byte bit 0-7 being 0)
   * @param bit 0-4 (D0-D4)
   * @param pressed true if pressed
   */
  void setKey(int line, int bit, bool pressed);

  /**
   * Read the input port
   * @param highByte The high byte of the port address (e.g. 0xFE, 0xFD...)
   * @return The data byte (D0-D4 contain key status). Bits set to 1 if NOT
   * pressed.
   */
  emulator_types::byte readPort(emulator_types::byte highByte);

  // Reset all keys
  void reset();
};

#endif // ZXEMULATOR_KEYBOARD_H
