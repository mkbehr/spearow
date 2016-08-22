#include "PulseUnit.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

PulseUnit::PulseUnit(float sampleRate)
  : sampleRate(sampleRate),
    dutyControl(0), enabled(0), time(0.0)
{
}

void PulseUnit::reset(void) {
  time = 0.0;
  // TODO reset envelope, maybe also duration and sweep?
  enabled = 1;
}

// why do some bytes get handled in one call here, but
// others get split into multiple calls? whatever.

void PulseUnit::write_sweep_control(uint8_t in) {
  // TODO move magic numbers
  sweepPeriod = (in & 0x70) >> 4;
  sweepNegate = !!(in & 0x08);
  sweepShift = in & 0x07;
}

uint8_t PulseUnit::read_sweep_control(void) {
  return (((sweepPeriod & 0x7) << 4) +
          (sweepNegate ? 0x08 : 0) +
          (sweepShift & 0x7));
}


void PulseUnit::write_duration_control(uint8_t in) {
  lengthCounterValue = in & 0x3f;
}

void PulseUnit::write_duty_control(uint8_t in) {
  dutyControl = in & 0x3;
}

// can't read duration control

uint8_t PulseUnit::read_duty_control(void) {
  return dutyControl & 0x3;
}


void PulseUnit::write_envelope_control(uint8_t in) {
  // TODO actually do something
  envelopeControl = in;
}

uint8_t PulseUnit::read_envelope_control(void) {
  return envelopeControl;
}

void PulseUnit::write_frequency_low(uint8_t in) {
  frequencyControl = (frequencyControl & 0xff00) + in;
}

void PulseUnit::write_frequency_high(uint8_t in) {
  frequencyControl = (frequencyControl & 0x00ff) + (in << 8);
}

// can't read frequency

void PulseUnit::write_duration_enable(bool in) {
  lengthCounterEnable = in;
}

bool PulseUnit::read_duration_enable(void) {
  return lengthCounterEnable;
}

float PulseUnit::period() {
  // TODO move magic number
  return (2048 - frequencyControl) * 8.0 / 4195304.0;
}

float PulseUnit::duty() {
  switch (dutyControl) {
  case 0:
    return 0.125;
  case 1:
    return 0.25;
  case 2:
    return 0.5;
  case 3:
    return 0.75;
  default:
    fprintf(stderr, "PulseUnit::duty(): Bad duty value %d\n", dutyControl);
    exit(-1);
  }
}

void PulseUnit::frameTick() {
  // this should be called at a rate of 512 Hz
  if ((frameStep % 2) == 0) { // length acts on 0, 2, 4, 6
    lengthCounterAct();
  }
  if (frameStep == 7) { // envelope acts on 7
    envelopeAct();
  }
  if ((frameStep % 4) == 2) { // sweep acts on 2, 6
    sweepAct();
  }
  frameStep = (frameStep + 1) % 7;
}

// void PulseUnit::updateSweep(bool _enabled, unsigned int _divider,
//                             unsigned int _shift, bool _negate) {
//   sweepEnabled = _enabled;
//   sweepDividerReload = _divider;
//   sweepShift = _shift;
//   sweepNegate = _negate;
//   sweepReset();
// }

// void PulseUnit::sweepReset() {
//   sweepDivider = sweepDividerReload;
//   // Note: resetting the sweep unit does not reset the divider.
// }

// void PulseUnit::updateEnvelope(bool loop, bool constant,
//                                unsigned char timerReload) {
//   // Note: this does not reset the envelope. reset() does that, which
//   // is called by writing to 0x4003 or 0x4007 (length counter load,
//   // timer high bits)
//   envelopeLoop = loop;
//   envelopeConstant = constant;
//   envelopeDividerReload = timerReload;
// }


void PulseUnit::sweepAct() {
  // TODO
}

// void PulseUnit::sweepAct() {
//   if (sweepDivider) {
//     sweepDivider--;
//     return;
//   }
//   sweepDivider = sweepDividerReload;
//   if (sweepEnabled) {
//     int dividerDelta = divider >> sweepShift;
//     // TODO: If the divider would go outside [MINIMUM_DIVIDER,
//     // MAXIMUM_DIVIDER], this should actually silence the channel
//     // but leave the divider unchanged. Uh, it also may be that we
//     // silence the channel before actually ticking, but as soon as
//     // we see that on our /next/ tick we will go outside the range.
//     if (sweepNegate) {
//       // TODO: If we are pulse channel 1, then we are actually
//       // adding the one's complement instead of the two's
//       // complement, so subtract one from dividerDelta. (But what
//       // happens if dividerDelta is zero - are we actually
//       // increasing the divider then???)

//       // dividerDelta <= divider, so this will never underflow
//       divider -= dividerDelta;
//     } else {
//       // not the correct check (divider should actually never exceed
//       // MAXIMUM_DIVIDER), but prevents overflow
//       if (divider <= PULSE_MAXIMUM_DIVIDER) {
//         divider += dividerDelta;
//       }
//     }
//   }
// }

// TODO
void PulseUnit::envelopeAct() {
}

// void PulseUnit::envelopeAct() {
//   if (envelopeDivider) {
//     envelopeDivider--;
//     return;
//   }
//   envelopeDivider = envelopeDividerReload;
//   if (envelopeCounter > 0) {
//     envelopeCounter--;
//   } else if (envelopeLoop) {
//     envelopeCounter = ENVELOPE_MAX;
//   }
// }

unsigned char PulseUnit::envelope() {
  // TODO
  return 1;
}

// unsigned char PulseUnit::envelope() {
//   unsigned char out = envelopeConstant ?
//     envelopeDividerReload : envelopeCounter;
//   assert((out >= 0) &&
//          (out <= ENVELOPE_MAX));
//   return out;
// }

void PulseUnit::lengthCounterAct() {
  // TODO
}

// void PulseUnit::lengthCounterAct() {
//   if ((!lengthCounterHalt) && (lengthCounterValue > 0)) {
//     lengthCounterValue--;
//   }
//   if (!enabled) {
//     // TODO check to see whether this is exactly the right behavior
//     lengthCounterValue = 0;
//   }
// }

// void PulseUnit::updateFrameCounter(bool mode) {
//   frameCounterMode = mode;
// }

// void PulseUnit::frameCounterQuarterFrame() {
//   sweepAct();
// }

// void PulseUnit::frameCounterHalfFrame() {
//   lengthCounterAct();
//   envelopeAct();
//   sweepAct();
// }

unsigned char PulseUnit::tick()
{
  float prd = period();
  float phase = fmod(((time - (0.125 * prd)) / prd), 1.0);
  if (phase < 0.0) {
    phase += 1.0;
  }
  unsigned char out = (phase < duty()) ? envelope() : 0;
  // if ((divider < PULSE_MINIMUM_DIVIDER) || (divider > PULSE_MAXIMUM_DIVIDER)) {
  //   out = 0;
  // }
  if (!enabled) {
    out = 0;
  }
  // if (!lengthCounterValue) {
  //   out = 0;
  // }
  time += 1.0 / sampleRate;
  return out;
}

// // Note: not guaranteed to print entire state
// void PulseUnit::printState(void) {
//   const char *enabledStr = enabled ? "enabled" : "disabled";
//   float frequency = 1.0 / period();
//   printf("Pulse wave channel %d: %s, duty %f, divider %d (%f Hz)\n",
//          // dummy channel number below
//          -1, enabledStr, duty, divider, frequency);
// }
