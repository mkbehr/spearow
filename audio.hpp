#ifndef AUDIO_H
#define AUDIO_H

#include <cstdlib>
#include <string>
#include <vector>

#include "portaudio.h"

#include "cpu.hpp"
#include "PulseUnit.hpp"
#include "CustomWaveUnit.hpp"

const int N_PULSE_UNITS = 2;
const int N_UNITS = 3;

const float SAMPLE_RATE = 44100.0;

class CPU;

class Audio {
public:
  Audio(CPU *cpu, float sampleRate = SAMPLE_RATE);
  ~Audio();
  void apuInit();
  float tick();
  void frameTick();


  float lastSample;

  std::vector<PulseUnit> pulses;
  CustomWaveUnit custom;

private:
  CPU *cpu;

  float time;
  float sampleRate;
  float timeStep;

  PaStream *stream;
};

#endif
