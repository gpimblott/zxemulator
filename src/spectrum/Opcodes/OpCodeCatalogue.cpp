// File: (OpCodeCatalogue.cpp)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "OpCodeCatalogue.h"
#include "ExtendedOpcodes.h"
#include "IndexOpcodes.h"
#include "instructions/ArithmeticOpcodes.h"
#include "instructions/BitOpcodes.h"
#include "instructions/IOOpcodes.h"
#include "instructions/InterruptOpcodes.h"
#include "instructions/JumpOpcodes.h"
#include "instructions/LoadOpcodes.h"
#include "instructions/LogicOpcodes.h"
#include "instructions/RotateOpcodes.h"
#include "instructions/StackOpcodes.h"

/**
 * Constructor to build all the opcode instances and add to the catalogue
 */
OpCodeCatalogue::OpCodeCatalogue() {
  add(new LoadOpcodes());
  add(new InterruptOpcodes());
  add(new LogicOpcodes());
  add(new JumpOpcodes());
  add(new IOOpcodes());
  add(new ExtendedOpcodes());
  add(new IndexOpcodes());
  add(new StackOpcodes());
  add(new BitOpcodes());
  add(new ArithmeticOpcodes());
  add(new RotateOpcodes());
}

OpCodeCatalogue::~OpCodeCatalogue() {
  for (OpCodeProvider *provider : providersList) {
    free(provider);
  }
}

/**
 * Add a new provider to the lookup list
 * @param opcode
 */
void OpCodeCatalogue::add(OpCodeProvider *provider) {
  providersList.push_back(provider);
};

/**
 * Find the specified opcode by asking each provider if it can process it
 *
 * NB : This is not optimal as we have to scan through each provider.  It would
 * be faster to build a master list of all opcodes and their functions
 *
 * @param opcode code to lookupOpcode
 * @return Either a pointer to the opcode class or null
 */
OpCode *OpCodeCatalogue::lookupOpcode(byte opcode) {
  for (OpCodeProvider *provider : providersList) {
    OpCode *code = provider->lookupOpcode(opcode);
    if (code != nullptr) {
      return code;
    }
  }
  return nullptr;
}
