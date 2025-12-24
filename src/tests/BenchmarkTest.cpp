#include "../spectrum/Processor.h"
#include "../utils/Logger.h"
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>

class PerformanceTest : public ::testing::Test {
protected:
  Processor processor;

  void SetUp() override {
    processor.init("roms/48k.bin");
    processor.setTurbo(true);
  }
};

TEST_F(PerformanceTest, MaxSpeedBenchmark) {
  // Run for exactly 1 second of REAL time and count frames/cycles
  auto start = std::chrono::high_resolution_clock::now();
  auto end = start + std::chrono::seconds(1);

  long frames = 0;

  std::cout << "Starting Benchmark..." << std::endl;

  while (std::chrono::high_resolution_clock::now() < end) {
    processor.executeFrame();
    frames++;
  }

  // Spectrum runs at 50 FPS (approx).
  // 69888 T-States per frame * 50 = 3.5M T-States/sec.

  long totalTStates = frames * 69888;
  double mhz = totalTStates / 1000000.0;

  std::cout << "Benchmark Results:" << std::endl;
  std::cout << "Frames executed: " << frames << std::endl;
  std::cout << "Effective Clock: " << mhz << " MHz" << std::endl;
  std::cout << "Speedup vs Real (3.5MHz): " << (mhz / 3.5) << "x" << std::endl;

  // Sanity check: Should be at least 1x (3.5MHz) if PC is decent
  ASSERT_GT(frames, 50);
}
