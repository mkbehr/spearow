#include "CustomWaveUnit.hpp"

#include <cassert>
#include <cmath>

CustomWaveUnit::CustomWaveUnit(float sampleRate)
  : frameStep(0), sampleRate(sampleRate),
    enabled(0),
    samples(CUSTOM_WAVE_SAMPLES, 0)
{
}

void CustomWaveUnit::loadSamples(std::vector<uint8_t> inSamples) {
  assert(inSamples.size() == (CUSTOM_WAVE_SAMPLES / 2));
  samples.clear();
  for (unsigned int i = 0; i < (CUSTOM_WAVE_SAMPLES / 2); i++) {
    samples.push_back(inSamples.at(i) >> 4);
    samples.push_back(inSamples.at(i) & 0x7);
  }
}

void CustomWaveUnit::reset(std::vector<uint8_t> inSamples) {
  // COMPAT make sure that the samples are loaded exactly when they
  // should be
  time = 0.0;
  loadSamples(inSamples);
  // TODO should this also reset duration?
}

// Write the enabled bit. Also pass in a pointer to a vector of input
// samples, which we'll load up if we enable the unit.
void CustomWaveUnit::write_enabled(bool in) {
  enabled = in;
}

bool CustomWaveUnit::read_enabled() {
  return enabled;
}

void CustomWaveUnit::write_duration(uint8_t in) {
  duration = 256 - in;
}

void CustomWaveUnit::write_envelope_control(uint8_t in) {
  envelopeControl = (in >> 5) & 0x3;
}

uint8_t CustomWaveUnit::read_envelope_control() {
  return (envelopeControl & 0x3) << 5;
}

void CustomWaveUnit::write_frequency_low(uint8_t in) {
  frequencyControl = (frequencyControl & 0xff00) + in;
}

void CustomWaveUnit::write_frequency_high(uint8_t in) {
  frequencyControl = (frequencyControl & 0x00ff) + (in << 8);
}

void CustomWaveUnit::write_duration_enable(bool in) {
  lengthCounterEnable = in;
}

bool CustomWaveUnit::read_duration_enable(void) {
  return lengthCounterEnable;
}

void CustomWaveUnit::frameTick() {
  // this should be called at a rate of 512 Hz
  if ((frameStep % 2) == 0) { // length acts on 0, 2, 4, 6
    lengthCounterAct();
  }
}

void CustomWaveUnit::lengthCounterAct() {
  if (duration) {
    duration--;
  }
  if (!duration) {
    enabled = 0;
  }
}

float CustomWaveUnit::period() {
  // TODO move magic number
  // the extra 8 sounds right, but I haven't confirmed why it's there
  return (2048 - frequencyControl) * 8.0 * 8.0 / 4195304.0;
}

uint8_t CustomWaveUnit::tick() {
  float prd = period();
  float phase = fmod((time - prd) / prd, 1.0);
  if (phase < 0.0) {
    phase += 1.0;
  }

  int sample_i = floor(phase * CUSTOM_WAVE_SAMPLES);

  uint8_t out = samples.at(sample_i);

  if (envelopeControl == 0) {
    out = 0;
  } else {
    out = out >> (envelopeControl - 1);
  }

  if (!enabled) {
    out = 0;
  }

  time += 1.0 / sampleRate;
  return out;
}
