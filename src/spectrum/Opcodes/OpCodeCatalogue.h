// File: (OpCodeCatalogue.h)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_OPCODECATALOGUE_H
#define ZXEMULATOR_OPCODECATALOGUE_H

#include "../../utils/BaseTypes.h"
#include "../ProcessorState.h"
#include "OpCode.h"
#include "OpCodeProvider.h"
#include "instructions/LoadOpcodes.h"
#include <list>

typedef std::list<OpCodeProvider *> providerList_t;

class OpCodeCatalogue {
private:
  providerList_t providersList;
  OpCode *m_opcodeLookup[256];

public:
  OpCodeCatalogue();

  void add(OpCodeProvider *provider);

  OpCode *lookupOpcode(byte opcode);

  virtual ~OpCodeCatalogue();
};

#endif // ZXEMULATOR_OPCODECATALOGUE_H
