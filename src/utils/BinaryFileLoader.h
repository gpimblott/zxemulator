//
// Created by gordo on 13/01/2023.
//

#ifndef ZXEMULATOR_BINARYFILELOADER_H
#define ZXEMULATOR_BINARYFILELOADER_H

#include <string>
#include "BaseTypes.h"

/**
 *
 */
class BinaryFileLoader {
protected:
    const char *filename = 0;
    long size = 0;
    emulator_types::byte *data = 0;
public:
    BinaryFileLoader(const char *filename);
    BinaryFileLoader(const char *filename, emulator_types::byte *buffer);
    emulator_types::byte operator [] (long i);

    // Utility routines to load a file
    static long getFileSize(const char *filename);
    static void readFileToBuffer(const char *filename, emulator_types::byte *buffer, int size);


};


#endif //ZXEMULATOR_BINARYFILELOADER_H
