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

#ifndef ZXEMULATOR_MEMORY_H
#define ZXEMULATOR_MEMORY_H

#include "../utils/BaseTypes.h"
#include "Rom.h"
#include "video/VideoBuffer.h"

#define ROM_LOCATION 0x0000

// Total address space = 64k
#define ROM_SIZE 0x4000 // 16K ROM
#define RAM_SIZE 0xC000 // 48K RAM

using namespace emulator_types;

/**
 *  Represents the ZX Spectrum memory
 *
 *  &0000 to &3FFF ROM (16K)
 *  &4000 to &57FF Screen memory
 *  &5800 to &5AFF Screen memory (colour data)
 *  &5B00 to &5BFF Printer Buffer
 *  &5C00 to &5CBF System variables
 *  &5CC0 to &5CCA Reserved
 *  &5CCB to &FF57 Available memory (between PROG and RAMTOP)
 *  &FF58 to &FFFF Reserved
 */
class Memory {
private:
  const long m_totalMemory = RAM_SIZE + ROM_SIZE;
  byte *m_memory;
  VideoBuffer *m_videoBuffer = nullptr;
  byte m_romScratch = 0; // Scratch byte for ROM write protection

public:
  Memory();

  ~Memory();

  void loadIntoMemory(long start, long length, byte *data);

  void loadIntoMemory(Rom &rom);

  // Get a word from the specified address
  word getWord(long address);

  // Get an instance of the video videoBuffer configured to point at the correct
  // location in this memory map
  VideoBuffer *getVideoBuffer();

  // Override operators
  emulator_types::byte &operator[](long i);

  void dump(long start, long size);
};

#endif // ZXEMULATOR_MEMORY_H
