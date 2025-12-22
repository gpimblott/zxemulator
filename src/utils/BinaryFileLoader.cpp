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
#include <iostream>
#include <sys/stat.h>

#include "BinaryFileLoader.h"

using namespace emulator_types;

/**
 * LoadOpcodes the specified file into a byte videoBuffer
 * @param filename Name of the file to load
 */
BinaryFileLoader::BinaryFileLoader(const char *filename) : filename(filename) {
  this->size = getFileSize(filename);
  if (this->size > 0) {
    this->data = new byte[this->size];
    readFileToBuffer(filename, this->data, this->size);
  }
}

/**
 * LoadOpcodes the file into a predefined videoBuffer
 * @param filename Name of the file to load
 * @param buffer Buffer tp load the file into
 */
BinaryFileLoader::BinaryFileLoader(const char *filename, byte *buffer)
    : filename(filename), data(buffer) {
  this->size = getFileSize(filename);
  if (this->size > 0) {
    readFileToBuffer(filename, this->data, this->size);
  }
}

/**
 * static routine to load the file into a videoBuffer
 * @return
 */
void BinaryFileLoader::readFileToBuffer(const char *filename, byte *buffer,
                                        int size) {
  if (buffer) {
    FILE *file_p = fopen(filename, "rb");
    int bytes_read = fread(buffer, sizeof(byte), size, file_p);
    printf("Read file %s - read %d of %d bytes\n", filename, size, bytes_read);
  }
}

/**
 * Static routine to get the size of the file
 * @return the size of the file or -1
 */
long BinaryFileLoader::getFileSize(const char *filename) {
  struct stat file_status;
  if (stat(filename, &file_status) >= 0) {
    return file_status.st_size;
  }
  return -1;
}

/**
 * Override the [] operators to allow direct access to bytes in the file
 * @param i index of the byte to return
 * @return the value of the byte or 0 if no data loaded
 */
byte BinaryFileLoader::operator[](long i) {
  if (data) {
    byte result = *(data + i);
    return result;
  } else {
    return 0;
  }
}
