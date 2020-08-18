#ifndef FUNCTIONS_WAV_TIME_H
#define FUNCTIONS_WAV_TIME_H

// Usage: WaveTimeIn, WaveTimeOut
// Returns 0-32768 time in [ms] of current in/out wav.
// returned value: INTEGER

class InWavTime {
 public:
  void run(BladeBase* blade) {
	value_ = clampi32(hybrid_font.in_wavtime*1000, 0, 32768);
  }
  int getInteger(int led) { return value_; }
  int value_;
};

class OutWavTime {
 public:
  void run(BladeBase* blade) {
	value_ = clampi32(hybrid_font.out_wavtime*1000, 0, 32768);

  }
  int getInteger(int led) { return value_; }
  int value_;
};

#endif
