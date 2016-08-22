#ifndef AUDIO_H
#define AUDIO_H

#include <cstdlib>
#include <string>
#include <vector>

#include "portaudio.h"

#include "cpu.hpp"
#include "PulseUnit.hpp"

const int N_PULSE_UNITS = 2;

const float SAMPLE_RATE = 44100.0;

class CPU;

class Audio {
public:
  Audio(CPU *cpu, float sampleRate = SAMPLE_RATE);
  ~Audio();
  void apuInit();
  float tick();
  void updateFrameCounter(bool);
  void frameCounterQuarterFrame();
  void frameCounterHalfFrame();
  // pulse wave interface
  void resetPulse(unsigned int);
  void setPulseDivider(unsigned int, unsigned int);
  void setPulseEnabled(unsigned int, bool);
  void setPulseDuty(unsigned int, float);
  void setPulseLengthCounterHalt(unsigned int, bool);
  void setPulseLengthCounter(unsigned int, unsigned int);
  void setPulseDuration(unsigned int, float);
  void updatePulseSweep(unsigned int pulse_n,
                        bool enabled, unsigned int divider,
                        unsigned int shift, bool negate);
  void updatePulseEnvelope(unsigned int pulse_n,
                           bool loop, bool constant,
                           unsigned char timerReload);

  float lastSample;

  std::vector<PulseUnit> pulses;

private:
  CPU *cpu;

  float time;
  float sampleRate;
  float timeStep;

  bool frameCounterMode;

  PaStream *stream;
};

#endif
