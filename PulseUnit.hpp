#ifndef PULSE_UNIT_H
#define PULSE_UNIT_H

#include <cstdint>

class PulseUnit {

public:

  PulseUnit(float sampleRate);
  void reset();
  void setDivider(unsigned int divider);
  void setDuty(float duty);
  void setEnabled(bool);
  void setLengthCounterHalt(bool halt);
  void setLengthCounter(unsigned int c);

  void updateSweep(bool enabled, unsigned int divider,
                   unsigned int shift, bool negate);
  void sweepReset();

  void updateEnvelope(bool loop, bool constant,
                      unsigned char timerReload);

  void updateFrameCounter(bool mode);
  void frameCounterQuarterFrame();
  void frameCounterHalfFrame();

  void frameTick();

  unsigned char tick();

  void printState(void);

  // SPEAROW: added things here
  // uint8_t sweep_control;
  // uint8_t duration_control;
  // uint8_t duty_control;
  // uint8_t envelope_control;
  // uint16_t frequency_control;
  // bool duration_enable;

  void write_sweep_control(uint8_t);
  void write_duration_control(uint8_t);
  void write_duty_control(uint8_t);
  void write_envelope_control(uint8_t);
  void write_frequency_low(uint8_t);
  void write_frequency_high(uint8_t);
  void write_duration_enable(bool);

  uint8_t read_sweep_control(void);
  // can't read duration control
  uint8_t read_duty_control(void);
  uint8_t read_envelope_control(void);
  // can't read frequency
  bool read_duration_enable(void);

protected:

  float period();
  void sweepAct();
  void envelopeAct();
  void lengthCounterAct();
  unsigned char envelope();

  const float sampleRate;

  int frameStep;

  unsigned int divider;
  unsigned int dutyControl;
  float duty();
  bool enabled;
  float time;

  bool lengthCounterEnable;
  int lengthCounterValue;

  uint16_t frequencyControl;

  // placeholder to store envelope byte until envelope functionality
  // is implemented
  uint8_t envelopeControl;

  bool envelopeLoop;
  bool envelopeConstant;
  // Note: the timer reload also specifies the envelope in constant mode
  unsigned char envelopeDividerReload;
  unsigned char envelopeDivider;
  unsigned char envelopeCounter;

  unsigned int sweepPeriod;
  unsigned int sweepDividerReload; // TODO maybe same as period
  unsigned int sweepDivider; // TODO implementation
  unsigned int sweepShift;
  bool sweepNegate;

  bool frameCounterMode;

};
#endif // PULSE_UNIT_H
