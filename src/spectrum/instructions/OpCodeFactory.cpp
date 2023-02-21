// File: (OpCodeFactory.cpp)
// Created by G.Pimblott on 19/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "OpCodeFactory.h"

OpCode *OpCodeFactory::createOpCode(emulator_types::byte magicNum, std::string name, executeFunc_t func) {
    OpCode *opcode = new OpCode(this->state, magicNum, name, func);
    return opcode;
}
