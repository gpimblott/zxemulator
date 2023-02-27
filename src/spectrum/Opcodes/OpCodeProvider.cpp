// File: (OpCodeProvider.cpp)
// Created by G.Pimblott on 19/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "OpCodeProvider.h"

OpCode *OpCodeProvider::lookupOpcode(byte opcode) {
    try {
        return opcodeLookup.at(opcode);
    } catch (std::exception ex) {
        return nullptr;
    }
}

OpCode *OpCodeProvider::createOpCode(emulator_types::byte magicNum, std::string name, executeFunc_t func) {
    OpCode *opcode = new OpCode(magicNum, name, func);
    opcodeLookup[magicNum] = opcode;
    return opcode;
}
