// File: (PeriodTimer.cpp)
// Created by G.Pimblott on 13/02/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "PeriodTimer.h"

void PeriodTimer::start() {
    // Get starting timepoint
    m_startTime = high_resolution_clock::now();
}

long PeriodTimer::stop() {
    auto stop = high_resolution_clock::now();
    m_duration = duration_cast<microseconds>(stop - m_startTime).count();
    return m_duration;
}

std::ostream &operator<<(std::ostream &os, const PeriodTimer &timer) {
    os << timer.m_duration << " microseconds ";
    return os;
}

long PeriodTimer::getDurationInMicroseconds() {
    return m_duration;
}
