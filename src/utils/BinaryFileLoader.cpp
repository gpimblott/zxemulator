/*
 * Copyright 2026 G.Pimblott
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
    // Expand ~ to home directory if needed
    std::string expandedPath = filename;
    if (expandedPath.length() > 0 && expandedPath[0] == '~') {
      const char *home = getenv("HOME");
      if (home) {
        expandedPath = std::string(home) + expandedPath.substr(1);
      }
    }

    FILE *file_p = fopen(expandedPath.c_str(), "rb");
    int bytes_read = fread(buffer, sizeof(byte), size, file_p);
    printf("Read file %s - read %d of %d bytes\n", filename, size, bytes_read);
    if (file_p) {
      fclose(file_p);
    }
  }
}

/**
 * Static routine to get the size of the file
 * @return the size of the file or -1
 */
long BinaryFileLoader::getFileSize(const char *filename) {
  // Expand ~ to home directory if needed
  std::string expandedPath = filename;
  if (expandedPath.length() > 0 && expandedPath[0] == '~') {
    const char *home = getenv("HOME");
    if (home) {
      expandedPath = std::string(home) + expandedPath.substr(1);
    }
  }

  struct stat file_status;
  if (stat(expandedPath.c_str(), &file_status) >= 0) {
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
