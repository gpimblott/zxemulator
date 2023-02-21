//
// Created by gordo on 16/01/2023.
//

#ifndef ZXEMULATOR_ROM_H
#define ZXEMULATOR_ROM_H

#include "../utils/BinaryFileLoader.h"
#include "../utils/BaseTypes.h"

class Rom : public BinaryFileLoader {

public:
    Rom(const char *filename);
    Rom(const char *filename, emulator_types::byte *buffer);

    long getSize();
    emulator_types::byte *getData();
};


#endif //ZXEMULATOR_ROM_H
