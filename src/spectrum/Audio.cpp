/*
 * MIT License
 *
 * Copyright (c) 2026 G.Pimblott
 */

#include "Audio.h"
#include <cmath>

Audio::Audio() {
  // Precise timing for 50Hz frame structure (69888 T-States per frame)
  // We need exactly 44100/50 = 882 samples per frame.
  // 69888 / 882 = 79.238095...
  tStatesPerSample = 69888.0 / ((double)SAMPLE_RATE / 50.0);
  tStateAccumulator = 0.0;

  // Reserve logical buffer size
  buffer.reserve(44100);

  std::vector<sf::SoundChannel> channelMap = {sf::SoundChannel::Mono};
  initialize(1, SAMPLE_RATE, channelMap);
}

Audio::~Audio() { stop(); }

void Audio::start() { play(); }

void Audio::reset() {
  stop();
  {
    std::lock_guard<std::mutex> lock(mutex);
    buffer.clear();
    samples.clear();
    // Pre-fill buffer with ~45ms of silence to start with cushion
    // This prevents immediate underrun if emulation thread is slightly delayed
    buffer.resize(2000, 0);
  }
  tStateAccumulator = 0;
  play();
}

void Audio::update(int tStates, bool speakerBit, bool earBit) {
  tStateAccumulator += tStates;

  while (tStateAccumulator >= tStatesPerSample) {
    tStateAccumulator -= tStatesPerSample;

    // Sample logic:
    // Speaker bit (0/1) + Ear bit (0/1)
    short sampleValue = 0;

    if (speakerBit)
      sampleValue += 20000;
    if (earBit)
      sampleValue += 8000;

    // Store in local pending buffer (no lock needed)
    pendingSamples.push_back(sampleValue);
  }

  // Auto-flush small chunks to keep the stream fluid
  if (pendingSamples.size() >= 100) {
    flush();
  }
}

void Audio::flush() {
  if (pendingSamples.empty())
    return;

  std::lock_guard<std::mutex> lock(mutex);
  buffer.insert(buffer.end(), pendingSamples.begin(), pendingSamples.end());
  pendingSamples.clear();
}

size_t Audio::getBufferSize() {
  std::lock_guard<std::mutex> lock(mutex);
  return buffer.size();
}

bool Audio::onGetData(Chunk &data) {
  std::lock_guard<std::mutex> lock(mutex);

  static std::int16_t lastSample = 0;

  // If buffer is empty (underrun), return last sample to maintain DC level
  if (buffer.empty()) {
    static std::int16_t underrunBuffer[10];
    std::fill(std::begin(underrunBuffer), std::end(underrunBuffer), lastSample);

    data.samples = underrunBuffer;
    data.sampleCount = 10;
    return true;
  }

  // Return all available data
  // Use swap to avoid copying data and minimize time holding the lock
  samples.swap(buffer);

  // Clear the buffer (now containing old samples data) to keep capacity
  buffer.clear();

  if (!samples.empty()) {
    lastSample = samples.back();
  }

  data.samples = samples.data();
  data.sampleCount = samples.size();

  return true;
}

void Audio::onSeek(sf::Time timeOffset) {
  // Not supported/needed for emulation stream
}
