/*
 * MIT License
 *
 * Copyright (c) 2026 G.Pimblott
 */

#ifndef ZXEMULATOR_AUDIO_H
#define ZXEMULATOR_AUDIO_H

#include "../utils/BaseTypes.h"
#include <SFML/Audio.hpp>
#include <cstdint>
#include <mutex>
#include <vector>

class Audio : public sf::SoundStream {
private:
  std::vector<std::int16_t> pendingSamples;
  std::vector<std::int16_t> samples;
  std::mutex mutex;
  std::vector<std::int16_t> buffer;

  double tStatesPerSample;
  double tStateAccumulator;

  // Approximate sample rate
  const unsigned int SAMPLE_RATE = 44100;
  // T-states per second approx 3.5MHz
  const double CPU_FREQUENCY = 3500000.0;

  virtual bool onGetData(Chunk &data) override;
  virtual void onSeek(sf::Time timeOffset) override;

public:
  Audio();
  virtual ~Audio();

  void update(int tStates, bool speakerBit, bool earBit);
  void flush();
  void start();
  void reset();
};

#endif // ZXEMULATOR_AUDIO_H
