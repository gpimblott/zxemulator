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

#include "Memory.h"
#include "../exceptions/MemoryException.h"
#include <cstdlib>

/**
 * Constructor
 * Allocate the memory
 */
/**
 * Constructor
 * Allocate the memory
 */
Memory::Memory() {
  m_memory = (byte *)calloc(1, m_totalMemory);
  m_videoBuffer = new VideoBuffer(m_memory);
}

/**
 * Destructor
 */
Memory::~Memory() {
  if (m_videoBuffer != nullptr) {
    delete m_videoBuffer;
    m_videoBuffer = nullptr;
  }
  if (m_memory != NULL) {
    free(m_memory);
    m_memory = NULL;
  }
}

/**
 * LoadOpcodes some data into memory
 * @param start Start location
 * @param length The length of data to load
 * @param data  The data to load
 */
void Memory::loadIntoMemory(long start, long length, byte *data) {
  memcpy(m_memory + start, data, length);
}

/**
 * LoadOpcodes a preloaded ROM into memory
 * @param start start address to load the ROM
 * @param rom The ROM to load
 */
void Memory::loadIntoMemory(Rom &rom) {
  loadIntoMemory(ROM_LOCATION, rom.getSize(), rom.getData());

  // Clear Video RAM explicitly to ensure clean start
  // Pixels: 0x4000, length 6144 (0x1800)
  memset(m_memory + 0x4000, 0, 6144);
  // Attributes: 0x5800, length 768 (0x300) -> 0x38 (White Paper, Black Ink)
  memset(m_memory + 0x5800, 0x38, 768);
}

/**
 * Override the [] operator to allow direct byte access to memory
 * @param i index of the byte to read
 * @return The byte value or throw a <code>MemoryExceptom</code>
 */
emulator_types::byte &Memory::operator[](long i) {
  if (i >= m_totalMemory)
    throw MemoryException(i);

  // ROM Protection (0x0000 - 0x3FFF)
  if (i < ROM_SIZE) {
    m_romScratch = m_memory[i]; // Load current ROM value into scratch
    return m_romScratch; // Return ref to scratch. Writes affect scratch, Reads
                         // get real value.
  }

  return *(this->m_memory + i);
}

/**
 * Debug routine to output chunks of memory
 * @param start start location
 * @param size number of bytes to dump
 */
void Memory::dump(long start, long size) {
  for (long i = 0; i < size; i++) {
    long address = start + i;
    if (i % 8 == 0)
      printf("\n%04ld ", address);
    printf("%hhx ", this->m_memory[address]);
  }
  printf("\n");
}

/**
 * Get a word from the specified address
 * @param address
 * @return The word at the specified address
 */
word Memory::getWord(long address) {
  word *ptr = reinterpret_cast<word *>(m_memory + address);
  return *(ptr);
}

/**
 *
 * @return
 */
VideoBuffer *Memory::getVideoBuffer() { return m_videoBuffer; }
