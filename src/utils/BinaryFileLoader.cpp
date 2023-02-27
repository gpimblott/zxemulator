//
// Created by gordo on 13/01/2023.
//
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
BinaryFileLoader::BinaryFileLoader(const char *filename, byte *buffer) : filename(filename), data(buffer) {
    this->size = getFileSize(filename);
    if (this->size > 0) {
        readFileToBuffer(filename, this->data, this->size);
    }
}

/**
 * static routine to load the file into a videoBuffer
 * @return
 */
void BinaryFileLoader::readFileToBuffer(const char *filename, byte *buffer, int size) {
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
