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

// SO1 is right, SO2 is left
const uint8_t CHANNEL_1_RIGHT = 1<<0;
const uint8_t CHANNEL_2_RIGHT = 1<<1;
const uint8_t CHANNEL_3_RIGHT = 1<<2;
const uint8_t CHANNEL_4_RIGHT = 1<<3;
const uint8_t CHANNEL_1_LEFT  = 1<<4;
const uint8_t CHANNEL_2_LEFT  = 1<<5;
const uint8_t CHANNEL_3_LEFT  = 1<<6;
const uint8_t CHANNEL_4_LEFT  = 1<<7;

class CPU;

class Audio {
public:
  Audio(CPU *cpu, float sampleRate = SAMPLE_RATE);
  ~Audio();
  void apuInit();
  void tick();
  void frameTick();


  float lastSampleLeft;
  float lastSampleRight;

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
