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
    *out++ = apu->lastSample;
    *out++ = apu->lastSample;
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

// Computes one sample. Returns the sample, and also stores it in the
// lastSample member. Automatically advances the stored time.
float Audio::tick(void) {
  float out = 0.0;
  for (int pulse_i = 0; pulse_i < N_PULSE_UNITS; pulse_i++) {
    // out += pulses[pulse_i].tick() * PULSE_MIX_COEFFICIENT;
    out += pulses[pulse_i].tick() / (15.0 * N_UNITS);
  }
  out += custom.tick() / (15.0 * N_UNITS);
  //out = (out * 2.0) - 1.0;
  lastSample = out;
  time += timeStep;
  return out;
}
