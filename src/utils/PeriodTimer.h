// File: (PeriodTimer.h)
// Created by G.Pimblott on 13/02/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_PERIODTIMER_H
#define ZXEMULATOR_PERIODTIMER_H

#include <ostream>
#include <chrono>

using namespace std::chrono;

class PeriodTimer {
private:
    high_resolution_clock::time_point m_startTime;
    long m_duration;

public:
    void start();
    long stop();
    long getDurationInMicroseconds();

    friend std::ostream &operator<<(std::ostream &os, const PeriodTimer &timer);

};


#endif //ZXEMULATOR_PERIODTIMER_H
