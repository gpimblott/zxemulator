// File: (OpCodeCatalogue.cpp)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "OpCodeCatalogue.h"
#include "InterruptsFactory.h"
#include "LoadFactory.h"
#include "LogicFactory.h"

using namespace OpCodes;

/**
 * Constructor to build all the opcode instances and add to the catalogue
 */
OpCodeCatalogue::OpCodeCatalogue(ProcessorState &state) {
    LoadFactory loadFactory(state);
    InterruptsFactory interruptsFactory(state);
    LogicFactory logicFactory(state);

    loadFactory.addToCatalogue(*this);
    interruptsFactory.addToCatalogue(*this);
    logicFactory.addToCatalogue(*this);

}

OpCodeCatalogue::~OpCodeCatalogue() {
    // @Todo Should free up all the Opcode classes here really
}

/**
 * Add a new opcode to the catalogue
 * @param opcode
 */
void OpCodeCatalogue::add(OpCode *opcode) {
    catalogue[opcode->getOpCode()] = opcode;
};

/**
 * Find the specified opcode and return its class
 * @param opcode code to lookup
 * @return Either a pointer to the opcode class or null
 */
OpCode *OpCodeCatalogue::lookup(emulator_types::byte opcode) {
    try {
        return catalogue.at(opcode);
    } catch (std::exception ex) {
        return nullptr;
    }
}
