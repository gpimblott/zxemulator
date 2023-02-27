// File: (debug.h)
// Created by G.Pimblott on 25/02/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_DEBUG_H
#define ZXEMULATOR_DEBUG_H

#define DEBUG 1

#ifdef DEBUG
#define debug(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...) ((void)0)
#endif

#endif //ZXEMULATOR_DEBUG_H
