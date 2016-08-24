#ifndef CUSTOM_WAVEFORM_UNIT_H
#define CUSTOM_WAVEFORM_UNIT_H

#include <cstdint>
#include <vector>

const unsigned int CUSTOM_WAVE_SAMPLES = 0x20;

class CustomWaveUnit {

public:

  CustomWaveUnit(float sampleRate);
  void loadSamples(std::vector<uint8_t>);

  void reset(std::vector<uint8_t>);

  void write_enabled(bool);
  bool read_enabled();

  void write_duration(uint8_t);
  // can't read duration

  void write_envelope_control(uint8_t);
  uint8_t read_envelope_control();

  void write_frequency_low(uint8_t);
  void write_frequency_high(uint8_t);
  void write_duration_enable(bool);
  // can't read frequency
  bool read_duration_enable(void);

  void frameTick();

  uint8_t tick();

private:

  float period();

  int frameStep;

  void lengthCounterAct();

  const float sampleRate;
  float time;
  bool enabled;
  uint8_t envelopeControl;
  uint8_t duration;
  bool lengthCounterEnable;
  uint16_t frequencyControl;
  std::vector<uint8_t> samples;
};

#endif // CUSTOM_WAVEFORM_UNIT_H
