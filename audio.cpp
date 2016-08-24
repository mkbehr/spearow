#include <stdexcept>
#include <string>

#include "Audio.hpp"

void checkPaError(PaError err) {
  if (err != paNoError) {
    fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
    exit(-1);
  }
}

static int apuCallback (const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo* timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData ) {
  Audio *apu = (Audio *) userData;
  float *out = (float *) outputBuffer;
  (void) inputBuffer; /* Prevent unused variable warning. */

  for(int i = 0; i < framesPerBuffer; i++) {
    *out++ = apu->lastSampleLeft;
    *out++ = apu->lastSampleRight;
    apu->tick();
  }
  return 0;
}

Audio::Audio(CPU *cpu, float sampleRate)
  : time(0), sampleRate(sampleRate), timeStep(1.0/sampleRate),
    pulses(std::vector<PulseUnit>(N_PULSE_UNITS, PulseUnit(sampleRate))),
    custom(sampleRate),
    cpu(cpu)
{
}

Audio::~Audio(void) {
  PaError err = Pa_StopStream(stream);
  checkPaError(err);

  err = Pa_Terminate();
  checkPaError(err);
}

void Audio::apuInit(void) {


  PaError err = Pa_Initialize();
  checkPaError(err);


  err = Pa_OpenDefaultStream(
    &stream,
    0,                /* no input channels */
    2,                /* stereo output */
    paFloat32,        /* 32 bit floating point output */
    (int) sampleRate, /* sample rate */
    256,              /* frames per buffer */
    apuCallback,      /* callback */
    this);            /* pointer passed to callback */
  checkPaError(err);

  err = Pa_StartStream(stream);
  checkPaError(err);

}

// Computes one sample. Stores it in lastSampleLeft and
// lastSampleRight. Automatically advances the stored time.
void Audio::tick(void) {
  float outLeft = 0.0;
  float outRight = 0.0;

  // TODO confirm how mixing works

  float channelOne = pulses[0].tick() / (15.0 * N_UNITS);
  if (cpu->audio_terminals & CHANNEL_1_LEFT) {
    outLeft += channelOne;
  }
  if (cpu->audio_terminals & CHANNEL_1_RIGHT) {
    outRight += channelOne;
  }

  float channelTwo = pulses[1].tick() / (15.0 * N_UNITS);
  if (cpu->audio_terminals & CHANNEL_2_LEFT) {
    outLeft += channelTwo;
  }
  if (cpu->audio_terminals & CHANNEL_2_RIGHT) {
    outRight += channelTwo;
  }

  float channelThree = custom.tick() / (15.0 * N_UNITS);
  if (cpu->audio_terminals & CHANNEL_3_LEFT) {
    outLeft += channelThree;
  }
  if (cpu->audio_terminals & CHANNEL_3_RIGHT) {
    outRight += channelThree;
  }

  // SO2
  float mixerLeftVolume = (((cpu->audio_volume >> 4) & 0x7) + 1) / 16.0;
  outLeft *= mixerLeftVolume;
  // SO1
  float mixerRightVolume = ((cpu->audio_volume & 0x7) + 1) / 16.0;
  outRight *= mixerRightVolume;

  lastSampleLeft = outLeft;
  lastSampleRight = outRight;
  time += timeStep;
}

void Audio::frameTick() {
  for (int pulse_i = 0; pulse_i < N_PULSE_UNITS; pulse_i++) {
    pulses[pulse_i].frameTick();
  }
  custom.frameTick();
}
