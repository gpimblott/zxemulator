// File: (LoadFactory.h)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_LOADFACTORY_H
#define ZXEMULATOR_LOADFACTORY_H

#include "../../utils/BaseTypes.h"
#include "OpCode.h"
#include "OpCodeFactory.h"

namespace OpCodes {

    class LoadFactory : public OpCodeFactory {
    private:
        static constexpr emulator_types::byte LD_DE_XX = 0x11;
        static int processLD_DE_XX(ProcessorState &state);

    public:
        LoadFactory(ProcessorState &state);

        void addToCatalogue(OpCodeCatalogue &catalogue) override;

    };

} // OpCodes

#endif //ZXEMULATOR_LOADFACTORY_H
