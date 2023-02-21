//
// Created by G.Pimblott on 16/01/2023.
//

#include "Rom.h"

/**
 * Constructor that loads the specified filename
 * @param filename
 */
Rom::Rom(const char *filename) : BinaryFileLoader(filename){
}

Rom::Rom(const char *filename, emulator_types::byte *buffer) : BinaryFileLoader( filename, buffer) {
}

long Rom::getSize() {
    return this->size;
}

emulator_types::byte *Rom::getData() {
    return this->data;
}


