/*
 Teensy Lightsaber Firmware
 http://fredrik.hubbe.net/lightsaber/teensy_saber.html
 Copyright (c) 2016 Fredrik Hubinette
 Additional copyright holders listed inline below.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

// Search for CONFIGURABLE in this file to find all the places which
// might need to be modified for your saber.


// Board version
#define VERSION_MAJOR 1
#define VERSION_MINOR 0

// If you have two 144 LED/m strips in your blade, connect
// both of them to bladePin and drive them in parallel.
const unsigned int maxLedsPerStrip = 144;


#if VERSION_MAJOR >= 2
#define V2
#endif

// TODO LIST:
// Make sure that sound is off before doing file command
// make "chargint style" prevent you from turning the saber "on"
// Audio work items:
//   Tune swings better
//   select clash from force
//   stab effect
//   synthesize swings
// Blade stuff
//    better clash
// Implement menues:
//    select sound font
//    select color
//    adjust volume
// POV-writer mode

// If your electonics inverts the bladePin for some reason, define this.
// #define INVERT_WS2811

// Feature defines, these let you turn off large blocks of code
// used for debugging.
#define ENABLE_AUDIO
#define ENABLE_MOTION
// #define ENABLE_SNOOZE
#define ENABLE_WS2811
// #define ENABLE_WATCHDOG
#define ENABLE_SD
#define ENABLE_SERIALFLASH

// If defined, DAC vref will be 3 volts, resulting in louder sound.
#define LOUD

// If defined all sound samples will be divided by 2, resulting in
// quieter sound.
// #define QUIET

// This doesn't seem  to work.
// #define TAR_UPLOADS_TO_SDCARD

// Use FTM timer for monopodws timing.
#define USE_FTM_TIMER

// You can get better SD card performance by
// activating the  USE_TEENSY3_OPTIMIZED_CODE define
// in SD.h in the teensy library, however, my sd card
// did not work with that define.

#include <Arduino.h>
#include <EEPROM.h>
#ifdef ENABLE_SERIALFLASH
#include <SerialFlash.h>
#endif

#include <DMAChannel.h>

#include <SD.h>

#include <SPI.h>
#include <Wire.h>
#include <math.h>


#ifdef ENABLE_SNOOZE
#include <Snooze.h>

SnoozeBlock snooze_config;
#endif


// Teensy 3.2 pin map:
// A lot of these can be changed, but be careful, because:
//   o The pins used by the prop shield cannot be easily changed.
//   o Pins that are going to control normal LEDs (not neopoxels) need PWM capability,
//     and not all teensy pins can do PWM.
//   o Touch input is not available on all pins.
//   o Sdcard chip select pin depends on what shield you use.
//   o Battery level and blade identification needs analog input, which is not possible
//     on all pins.
//
// Note that while WS2811 leds/strips are being controlled, analogWriteFrequency() can only
// be used on pins 3, 4, 25 & 32, on all others the analogWriteFrequency is locked to the
// frequency of the WS2811 (usually 740kHz) which is not an approperiate frequency for
// driving a FET. This limitation may change in future versions of this software, but for
// now, if you want to drive a crystal chamber or other internal LEDs, I recommend using
// the pins listed above to get a reasonable PWM frequency.
//
// See the teensy 3.2 pinout diagram for more info: https://www.pjrc.com/teensy/pinout.html
enum SaberPins {
  // Bottom edge (in pin-out diagram)
  sdCardSelectPin = 0,            // SD card chip (sd card adapter)
                                  // Set to BUILTIN_SDCARD for Teensy 3.5/3.6
                                  // (See File->Example->SD->ReadWrite for
                                  // other possible values.)
#ifdef V2
  amplifierPin = 1,               // Amplifier enable pin (TeensySaber V2)
  motionSensorInterruptPin = 2,   // motion sensor interrupt (TeensySaber V2)
  bladePowerPin4 = 3,             // Optional power control (TeensySaber V2)
  bladePowerPin5 = 4,             // Optional power control (TeensySaber V2)
  bladePowerPin6 = 5,             // Optional power control (TeensySaber V2)
  freePin6 = 6,
  freePin7 = 7,
#else
  freePin1 = 1,                   // FREE
  motionSensorInterruptPin = 2,   // motion sensor interrupt (prop shield)
  freePin3 = 3,                   // FREE
  freePin4 = 4,                   // FREE (make this sdCardSelectPin if you're using a Wiz820+SD shield)
  amplifierPin = 5,               // Amplifier enable pin (prop shield)
  serialFlashSelectPin = 6,       // serial flash chip select (prop shield)
  spiLedSelect = 7,               // APA102/dotstar chip select (prop shield)
#endif
  freePin8 = 8,                   // FREE
  freePin9 = 9,                   // FREE
  freePin10 = 10,                 // FREE
  spiDataOut = 11,                // spi out, serial flash, spi led & sd card
  spiDataIn = 12,                 // spi in, serial flash & sd card

  // Top edge
  spiClock = 13,                  // spi clock, flash, spi led & sd card
  batteryLevelPin = 14,           // battery level input
  auxPin = 15,                    // AUX button
  powerButtonPin = 16,            // power button
  aux2Pin = 17,                   // AUX2 button
  i2cDataPin = 18,                // I2C bus, Used by motion sensors
  i2cClockPin = 19,               // I2C bus, Used by motion sensors
  bladePin = 20,                  // blade control, either WS2811 or PWM
  bladeIdentifyPin = 20,          // blade identify input / FoC
  bladePowerPin1 = 21,            // blade power control
  bladePowerPin2 = 22,            // blade power control
  bladePowerPin3 = 23,            // blade power control
};


const char version[] = "$Id$";

/*
 * State machine code.
 * This code uses the fact that switch can jump into code to create
 * lightweight pseudo-thread. These threads have a lot of limitations,
 * but they can make code much easier to read compared to manually
 * written state machines. Lots of examples below.
 */
// Note, you cannot have two YIELD() on the same line.
#define YIELD() do { next_state_ = __LINE__; return; case __LINE__: break; } while(0)
#define SLEEP(MILLIS) do { sleep_until_ = millis() + (MILLIS); while (millis() < sleep_until_) YIELD(); } while(0)
#define STATE_MACHINE_BEGIN() switch(next_state_) { case -1:
#define STATE_MACHINE_END() }

#define NELEM(X) (sizeof(X)/sizeof((X)[0]))

class StateMachine {
protected:
  int next_state_ = -1;
  uint32_t sleep_until_ = 0;
  void reset_state_machine() {
    next_state_ = -1;
  }
};

// Magic type used to prevent linked-list types from automatically linking.
enum NoLink { NOLINK = 17 };

// Helper class for classses that needs to be called back from the Loop() function.
class Looper;
Looper* loopers = NULL;
class Looper {
public:
  void Link() {
    next_looper_ = loopers;
    loopers = this;
  }
  void Unlink() {
    for (Looper** i = &loopers; *i; i = &(*i)->next_looper_) {
      if (*i == this) {
        *i = next_looper_;
        return;
      }
    }
  }
  Looper() { Link(); }
  explicit Looper(NoLink _) { }
  ~Looper() { Unlink(); }
  static void DoLoop() {
    for (Looper *l = loopers; l; l = l->next_looper_) {
      l->Loop();
    }
  }
  static void DoSetup() {
    for (Looper *l = loopers; l; l = l->next_looper_) {
      l->Setup();
    }
  }
protected:
  virtual void Loop() = 0;
  virtual void Setup() {}
private:
  Looper* next_looper_;
};

// Command parsing linked list base class.
class CommandParser;
CommandParser* parsers = NULL;

class CommandParser {
public:
  void Link() {
    next_parser_ = parsers;
    parsers = this;
  }
  void Unlink() {
    for (CommandParser** i = &parsers; *i; i = &(*i)->next_parser_) {
      if (*i == this) {
        *i = next_parser_;
        return;
      }
    }
  }

  CommandParser() { Link(); }
  explicit CommandParser(NoLink _) {}
  ~CommandParser() { Unlink(); }
  static bool DoParse(const char* cmd, const char* arg) {
    for (CommandParser *p = parsers; p; p = p->next_parser_) {
      if (p->Parse(cmd, arg))
        return true;
    }
    return false;
  }
protected:
  virtual bool Parse(const char* cmd, const char* arg) = 0;
private:
  CommandParser* next_parser_;
};

class SaberBase;
SaberBase* saberbases = NULL;

class SaberBase {
public:
  void Link() {
    next_saber_ = saberbases;
    saberbases = this;
  }
  void Unlink() {
    for (SaberBase** i = &saberbases; *i; i = &(*i)->next_saber_) {
      if (*i == this) {
        *i = next_saber_;
        return;
      }
    }
  }

  SaberBase() { Link(); }
  explicit SaberBase(NoLink _) {}
  ~SaberBase() { Unlink(); }


#define SABERFUN(NAME, TYPED_ARGS, ARGS)			\
public:								\
  static void Do##NAME TYPED_ARGS {				\
    for (SaberBase *p = saberbases; p; p = p->next_saber_) {	\
      p->NAME ARGS;						\
    }								\
  }								\
protected:							\
  virtual void NAME TYPED_ARGS {}

  SABERFUN(Clash, (), ());
  SABERFUN(Stab, (), ());
  SABERFUN(On, (), ());
  SABERFUN(Off, (), ());
  SABERFUN(Lockup, (), ());
  SABERFUN(Force, (), ());
  SABERFUN(Blast, (), ());
  SABERFUN(Boot, (), ());
  SABERFUN(NewFont, (), ());
  SABERFUN(BeginLockup, (), ());
  SABERFUN(EndLockup, (), ());

  // Swing rotation speed
  SABERFUN(Motion, (float speed), (speed));

  // Wrist rotation speed
  SABERFUN(XMotion, (float speed), (speed));

  SABERFUN(Top, (), ());
  SABERFUN(IsOn, (bool* on), (on));

private:
  SaberBase* next_saber_;
};


// Debug printout helper class
class Monitoring : Looper, CommandParser {
public:
  Monitoring() : Looper(), CommandParser() {}
  enum MonitorBit {
    MonitorSwings = 1,
    MonitorSamples = 2,
    MonitorTouch = 4,
    MonitorBattery = 8,
  };

  bool ShouldPrint(MonitorBit bit) {
    return monitor_this_loop_ && (bit & active_monitors_);
  }
protected:
  void Loop() override {
    monitor_this_loop_ = millis() - last_monitor_loop_ > monitor_frequency_ms_;
    if (monitor_this_loop_) {
      last_monitor_loop_ = millis();
    }
  }
  bool Parse(const char *cmd, const char* arg) override {
    if (!strcmp(cmd, "monitor") || !strcmp(cmd, "mon")) {
      if (!strcmp(arg, "swings")) {
        active_monitors_ ^= MonitorSwings;
        return true;
      }
      if (!strcmp(arg, "samples")) {
        active_monitors_ ^= MonitorSamples;
        return true;
      }
      if (!strcmp(arg, "touch")) {
        active_monitors_ ^= MonitorTouch;
        return true;
      }
      if (!strcmp(arg, "battery")) {
        active_monitors_ ^= MonitorBattery;
        return true;
      }
    }
    return false;
  }
private:
  uint32_t monitor_frequency_ms_ = 500;
  int last_monitor_loop_ = 0;
  bool monitor_this_loop_ = false;
  uint32_t active_monitors_ = 0;
};

Monitoring monitor;

// Returns the decimals of a number, ie 12.2134 -> 0.2134
float fract(float x) { return x - floor(x); }

// clamp(x, a, b) makes sure that x is between a and b.
float clamp(float x, float a, float b) {
  if (x < a) return a;
  if (x > b) return b;
  return x;
}
int32_t clampi32(int32_t x, int32_t a, int32_t b) {
  if (x < a) return a;
  if (x > b) return b;
  return x;
}

const int16_t sin_table[1024] = {
  0,100,201,301,402,502,603,703,803,904,1004,1104,1205,1305,1405,
  1505,1605,1705,1805,1905,2005,2105,2204,2304,2403,2503,2602,2701,
  2800,2899,2998,3097,3196,3294,3393,3491,3589,3687,3785,3883,3980,
  4078,4175,4272,4369,4466,4562,4659,4755,4851,4947,5043,5139,5234,
  5329,5424,5519,5613,5708,5802,5896,5989,6083,6176,6269,6362,6454,
  6547,6639,6730,6822,6913,7004,7095,7185,7276,7365,7455,7545,7634,
  7722,7811,7899,7987,8075,8162,8249,8336,8422,8508,8594,8679,8764,
  8849,8934,9018,9101,9185,9268,9351,9433,9515,9597,9678,9759,9839,
  9920,9999,10079,10158,10237,10315,10393,10470,10547,10624,10700,
  10776,10852,10927,11002,11076,11150,11223,11296,11369,11441,11513,
  11584,11655,11725,11795,11865,11934,12003,12071,12139,12206,12273,
  12339,12405,12470,12535,12600,12664,12727,12790,12853,12915,12977,
  13038,13098,13158,13218,13277,13336,13394,13452,13509,13565,13621,
  13677,13732,13787,13841,13894,13947,14000,14052,14103,14154,14204,
  14254,14303,14352,14400,14448,14495,14542,14588,14633,14678,14722,
  14766,14810,14852,14894,14936,14977,15017,15057,15097,15135,15174,
  15211,15248,15285,15321,15356,15391,15425,15458,15491,15524,15556,
  15587,15618,15648,15677,15706,15734,15762,15789,15816,15841,15867,
  15892,15916,15939,15962,15984,16006,16027,16048,16068,16087,16106,
  16124,16141,16158,16174,16190,16205,16220,16233,16247,16259,16271,
  16283,16293,16304,16313,16322,16330,16338,16345,16352,16358,16363,
  16367,16371,16375,16378,16380,16381,16382,16383,16382,16381,16380,
  16378,16375,16371,16367,16363,16358,16352,16345,16338,16330,16322,
  16313,16304,16293,16283,16271,16259,16247,16233,16220,16205,16190,
  16174,16158,16141,16124,16106,16087,16068,16048,16027,16006,15984,
  15962,15939,15916,15892,15867,15841,15816,15789,15762,15734,15706,
  15677,15648,15618,15587,15556,15524,15491,15458,15425,15391,15356,
  15321,15285,15248,15211,15174,15135,15097,15057,15017,14977,14936,
  14894,14852,14810,14766,14722,14678,14633,14588,14542,14495,14448,
  14400,14352,14303,14254,14204,14154,14103,14052,14000,13947,13894,
  13841,13787,13732,13677,13621,13565,13509,13452,13394,13336,13277,
  13218,13158,13098,13038,12977,12915,12853,12790,12727,12664,12600,
  12535,12470,12405,12339,12273,12206,12139,12071,12003,11934,11865,
  11795,11725,11655,11584,11513,11441,11369,11296,11223,11150,11076,
  11002,10927,10852,10776,10700,10624,10547,10470,10393,10315,10237,
  10158,10079,9999,9920,9839,9759,9678,9597,9515,9433,9351,9268,9185,
  9101,9018,8934,8849,8764,8679,8594,8508,8422,8336,8249,8162,8075,
  7987,7899,7811,7722,7634,7545,7455,7365,7276,7185,7095,7004,6913,
  6822,6730,6639,6547,6454,6362,6269,6176,6083,5989,5896,5802,5708,
  5613,5519,5424,5329,5234,5139,5043,4947,4851,4755,4659,4562,4466,
  4369,4272,4175,4078,3980,3883,3785,3687,3589,3491,3393,3294,3196,
  3097,2998,2899,2800,2701,2602,2503,2403,2304,2204,2105,2005,1905,
  1805,1705,1605,1505,1405,1305,1205,1104,1004,904,803,703,603,502,
  402,301,201,100,0,-100,-201,-301,-402,-502,-603,-703,-803,-904,
  -1004,-1104,-1205,-1305,-1405,-1505,-1605,-1705,-1805,-1905,-2005,
  -2105,-2204,-2304,-2403,-2503,-2602,-2701,-2800,-2899,-2998,-3097,
  -3196,-3294,-3393,-3491,-3589,-3687,-3785,-3883,-3980,-4078,-4175,
  -4272,-4369,-4466,-4562,-4659,-4755,-4851,-4947,-5043,-5139,-5234,
  -5329,-5424,-5519,-5613,-5708,-5802,-5896,-5989,-6083,-6176,-6269,
  -6362,-6454,-6547,-6639,-6730,-6822,-6913,-7004,-7095,-7185,-7276,
  -7365,-7455,-7545,-7634,-7722,-7811,-7899,-7987,-8075,-8162,-8249,
  -8336,-8422,-8508,-8594,-8679,-8764,-8849,-8934,-9018,-9101,-9185,
  -9268,-9351,-9433,-9515,-9597,-9678,-9759,-9839,-9920,-9999,-10079,
  -10158,-10237,-10315,-10393,-10470,-10547,-10624,-10700,-10776,-10852,
  -10927,-11002,-11076,-11150,-11223,-11296,-11369,-11441,-11513,-11584,
  -11655,-11725,-11795,-11865,-11934,-12003,-12071,-12139,-12206,-12273,
  -12339,-12405,-12470,-12535,-12600,-12664,-12727,-12790,-12853,-12915,
  -12977,-13038,-13098,-13158,-13218,-13277,-13336,-13394,-13452,-13509,
  -13565,-13621,-13677,-13732,-13787,-13841,-13894,-13947,-14000,-14052,
  -14103,-14154,-14204,-14254,-14303,-14352,-14400,-14448,-14495,-14542,
  -14588,-14633,-14678,-14722,-14766,-14810,-14852,-14894,-14936,-14977,
  -15017,-15057,-15097,-15135,-15174,-15211,-15248,-15285,-15321,-15356,
  -15391,-15425,-15458,-15491,-15524,-15556,-15587,-15618,-15648,-15677,
  -15706,-15734,-15762,-15789,-15816,-15841,-15867,-15892,-15916,-15939,
  -15962,-15984,-16006,-16027,-16048,-16068,-16087,-16106,-16124,-16141,
  -16158,-16174,-16190,-16205,-16220,-16233,-16247,-16259,-16271,-16283,
  -16293,-16304,-16313,-16322,-16330,-16338,-16345,-16352,-16358,-16363,
  -16367,-16371,-16375,-16378,-16380,-16381,-16382,-16383,-16382,-16381,
  -16380,-16378,-16375,-16371,-16367,-16363,-16358,-16352,-16345,-16338,
  -16330,-16322,-16313,-16304,-16293,-16283,-16271,-16259,-16247,-16233,
  -16220,-16205,-16190,-16174,-16158,-16141,-16124,-16106,-16087,-16068,
  -16048,-16027,-16006,-15984,-15962,-15939,-15916,-15892,-15867,-15841,
  -15816,-15789,-15762,-15734,-15706,-15677,-15648,-15618,-15587,-15556,
  -15524,-15491,-15458,-15425,-15391,-15356,-15321,-15285,-15248,-15211,
  -15174,-15135,-15097,-15057,-15017,-14977,-14936,-14894,-14852,-14810,
  -14766,-14722,-14678,-14633,-14588,-14542,-14495,-14448,-14400,-14352,
  -14303,-14254,-14204,-14154,-14103,-14052,-14000,-13947,-13894,-13841,
  -13787,-13732,-13677,-13621,-13565,-13509,-13452,-13394,-13336,-13277,
  -13218,-13158,-13098,-13038,-12977,-12915,-12853,-12790,-12727,-12664,
  -12600,-12535,-12470,-12405,-12339,-12273,-12206,-12139,-12071,-12003,
  -11934,-11865,-11795,-11725,-11655,-11584,-11513,-11441,-11369,-11296,
  -11223,-11150,-11076,-11002,-10927,-10852,-10776,-10700,-10624,-10547,
  -10470,-10393,-10315,-10237,-10158,-10079,-9999,-9920,-9839,-9759,-9678,
  -9597,-9515,-9433,-9351,-9268,-9185,-9101,-9018,-8934,-8849,-8764,-8679,
  -8594,-8508,-8422,-8336,-8249,-8162,-8075,-7987,-7899,-7811,-7722,-7634,
  -7545,-7455,-7365,-7276,-7185,-7095,-7004,-6913,-6822,-6730,-6639,-6547,
  -6454,-6362,-6269,-6176,-6083,-5989,-5896,-5802,-5708,-5613,-5519,-5424,
  -5329,-5234,-5139,-5043,-4947,-4851,-4755,-4659,-4562,-4466,-4369,-4272,
  -4175,-4078,-3980,-3883,-3785,-3687,-3589,-3491,-3393,-3294,-3196,-3097,
  -2998,-2899,-2800,-2701,-2602,-2503,-2403,-2304,-2204,-2105,-2005,-1905,
  -1805,-1705,-1605,-1505,-1405,-1305,-1205,-1104,-1004,-904,-803,-703,
  -603,-502,-402,-301,-201,-100
};

#ifdef ENABLE_AUDIO

class ClickAvoiderLin {
public:
  ClickAvoiderLin() : speed_(0) { }
  ClickAvoiderLin(uint32_t speed) : speed_(speed) { }
  void set_target(uint32_t target) { target_ = target; }
  void set_speed(uint32_t speed) { speed_ = speed; }
  void set(uint16_t v) { current_ = v; }
  uint32_t value() const {return current_; }
  void advance() {
    uint32_t target = target_;
    if (current_ > target) {
      current_ -= min(speed_, current_ - target);
      return;
    }
    if (current_ < target) {
      current_ += min(speed_, target - current_);
      return;
    }
  }

  uint32_t speed_;
  uint32_t current_;
  uint32_t target_;
};

struct WaveForm {
  int16_t table_[1024];
};

struct WaveFormSampler {
  WaveFormSampler(const WaveForm& waveform) : waveform_(waveform.table_), pos_(0), delta_(0) {}
  WaveFormSampler(const int16_t* waveform) : waveform_(waveform), pos_(0), delta_(0) {}
  const int16_t *waveform_;
  int pos_;
  volatile int delta_;
  int16_t next() {
    pos_ += delta_;
    if (pos_ > 1024 * 65536) pos_ -= 1024 * 65536;
    // Bilinear lookup here?
    return waveform_[pos_ >> 16];
  }
};

template<class T>
class DataStream {
public:
  virtual int read(T* data, int elements) = 0;
  // There is no need to call eof() unless read() returns zero elements.
  virtual bool eof() { return false; }
};

#define AUDIO_BUFFER_SIZE 44
#define AUDIO_RATE 44100

#define PDB_CONFIG (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT | PDB_SC_PDBIE | PDB_SC_DMAEN)

class DAC {
public:
  DAC() {
    dma.begin(true); // Allocate the DMA channel first
    SIM_SCGC2 |= SIM_SCGC2_DAC0;
    DAC0_C0 = DAC_C0_DACEN;
#ifdef LOUD
    DAC0_C0 |= DAC_C0_DACRFS;  // 3.3V, much louder
#endif
    // This would cause a click, but the amp is not on yet...
    *(int16_t *)&(DAC0_DAT0L) = 2048;

    // set the programmable delay block to trigger DMA requests
    SIM_SCGC6 |= SIM_SCGC6_PDB;
    PDB0_IDLY = 1;
    PDB0_MOD = F_BUS / AUDIO_RATE;
    PDB0_SC = PDB_CONFIG | PDB_SC_LDOK;
    PDB0_SC = PDB_CONFIG | PDB_SC_SWTRIG;
    PDB0_CH0C1 = 0x0101;

    dma.TCD->SADDR = dac_dma_buffer;
    dma.TCD->SOFF = 2;
    dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
    dma.TCD->NBYTES_MLNO = 2;
    dma.TCD->SLAST = -sizeof(dac_dma_buffer);
    dma.TCD->DADDR = &DAC0_DAT0L;
    dma.TCD->DOFF = 0;
    dma.TCD->CITER_ELINKNO = NELEM(dac_dma_buffer);
    dma.TCD->DLASTSGA = 0;
    dma.TCD->BITER_ELINKNO = NELEM(dac_dma_buffer);
    dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
    dma.triggerAtHardwareEvent(DMAMUX_SOURCE_PDB);
    dma.enable();
    dma.attachInterrupt(isr);
  }

  void SetStream(DataStream<int16_t>* stream) {
    stream_ = stream;
  }

private:
  static void isr(void) {
    int16_t *dest, *end;
    uint32_t saddr;

    saddr = (uint32_t)(dma.TCD->SADDR);
    dma.clearInterrupt();
    if (saddr < (uint32_t)dac_dma_buffer + sizeof(dac_dma_buffer) / 2) {
      // DMA is transmitting the first half of the buffer
      // so we must fill the second half
      dest = (int16_t *)&dac_dma_buffer[AUDIO_BUFFER_SIZE];
      end = (int16_t *)&dac_dma_buffer[AUDIO_BUFFER_SIZE*2];
    } else {
      // DMA is transmitting the second half of the buffer
      // so we must fill the first half
      dest = (int16_t *)dac_dma_buffer;
      end = (int16_t *)&dac_dma_buffer[AUDIO_BUFFER_SIZE];
    }
    DataStream<int16_t> *stream = stream_;
    if (stream) {
      int n = stream->read(dest, end-dest);
      while (n--) {
	*dest = ((*(int16_t*)dest) + 32767) >> 4;
        dest++;
      }
    }
    while (dest < end) { *dest++ = 2047; }
  }

public: // FIXME remove 'public'
  DMAMEM static uint16_t dac_dma_buffer[AUDIO_BUFFER_SIZE*2];
  static DataStream<int16_t> * volatile stream_;
  static DMAChannel dma;
};

DMAChannel DAC::dma(false);
DataStream<int16_t> * volatile DAC::stream_ = nullptr;
DMAMEM uint16_t DAC::dac_dma_buffer[AUDIO_BUFFER_SIZE*2];

DAC dac;

template<int N> class AudioDynamicMixer : public DataStream<int16_t>, Looper {
public:
  AudioDynamicMixer() {
    for (int i = 0; i < N; i++) {
      streams_[i] = nullptr;
    }
  }

  // Calculate square root of |x|, using the previous square
  // root as a guess.
  int my_sqrt(int x) {
    if (x <= 0) return 0;
    int over, under, step = 1;
    if (last_square_ * last_square_ > x) {
      over = last_square_;
      under = over - 1;
      while (under * under > x) {
	over = under;
	under -= step;
	step += step;
	if (under <= 0) { under = 0; break; }
      }
    } else {
      under = last_square_;
      over = under + 1;
      while (over * over <= x) {
	under = over;
	over += step;
	step += step;
	if (over < 0) { over = x; break; }
      }
    }
    while (under + 1 < over) {
      int mid = (over + under) >> 1;
      if (mid * mid > x) {
	over = mid;
      } else {
	under = mid;
      }
    }
    return last_square_ = under;
  }
  int last_square_ = 0;

  int read(int16_t* data, int elements) override {
    int32_t sum[32];
    int ret = elements;
    int v = 0, v2 = 0;
    while (elements) {
      int to_do = min(elements, (int)NELEM(sum));
      for (int i = 0; i < to_do; i++) sum[i] = 0;
      for (int i = 0; i < N; i++) {
	int e = streams_[i] ? streams_[i]->read(data, to_do) : 0;
	for (int j = 0; j < e; j++) {
	  sum[j] += data[j];
	}
      }

      for (int i = 0; i < to_do; i++) {
	v = sum[i];
	vol_ = ((vol_ + abs(v)) * 255) >> 8;
	v2 = (v << 10) / (my_sqrt(vol_) + 100);
#ifdef QUIET
	v2 >>= 1;
#endif
	data[i] = clampi32(v2, -32768, 32767);
	peak_sum_ = max(abs(v), peak_sum_);
	peak_ = max(abs(v2), peak_);
      }
      data += to_do;
      elements -= to_do;
    }
    last_sample_ = v2;
    last_sum_ = v;
    
//    Serial.println(vol_);
    return ret;
  }

  void Loop() override {
    if (monitor.ShouldPrint(Monitoring::MonitorSamples)) {
      Serial.print("Mixer volume: ");
      Serial.print(vol_);
      Serial.print(" last sum: ");
      Serial.print(last_sum_);
      Serial.print(" last sample:");
      Serial.print(last_sample_);
      Serial.print(" peak sum: ");
      Serial.print(peak_sum_);
      Serial.print(" peak: ");
      Serial.println(peak_);
      peak_sum_ = peak_ = 0;
    }
  }

  // TODO: Make levels monitorable
  
  DataStream<int16_t>* streams_[N];
  int32_t vol_ = 0;
  int32_t last_sample_ = 0;
  int32_t last_sum_ = 0;
  int32_t peak_sum_ = 0;
  int32_t peak_ = 0;
//  int32_t sum_;
//  ClickAvoiderLin volume_;
};

AudioDynamicMixer<7> dynamic_mixer;

class Beeper : public DataStream<int16_t> {
public:
  int read(int16_t *data, int elements) override {
    int e = elements;
    while (true) {
      int s = min(elements, samples_);
      s = min(s, x_);
      if (s <= 0) return e - s;
      if (up_) {
	for (int i = 0; i < s; i++) data[i] = 2000;
      } else {
	for (int i = 0; i < s; i++) data[i] = -2000;
      }
      data += s;
      elements -= s;
      x_ -= s;
      samples_ -= s;
      if (x_ == 0) {
	x_ = f_;
	up_ = !up_;
      }
    }
  }

  void Beep(float length, float freq) {
    x_ = f_ = AUDIO_RATE / freq / 2.0;
    samples_ = AUDIO_RATE * length;
  }

  bool isPlaying() {
    return samples_ > 0;
  }

private:  
  volatile int samples_ = 0;
  volatile int f_ = 0;
  volatile int x_ = 0;
  volatile bool up_ = false;
};

Beeper beeper;

class LightSaberSynth : public DataStream<int16_t>, Looper {
public:
//  WaveForm sin_a_;
//  WaveForm sin_b_;
  WaveForm buzz_;
  WaveForm humm_;
  WaveFormSampler sin_sampler_a_hi_;
  WaveFormSampler sin_sampler_a_lo_;
  WaveFormSampler sin_sampler_b_;
  WaveFormSampler buzz_sampler_;
  WaveFormSampler humm_sampler_hi_;
  WaveFormSampler humm_sampler_lo_;
  ClickAvoiderLin volume_;

  // For debug monitoring..
  int32_t last_value = 0;
  int32_t last_prevolume_value = 0;
  int32_t last_elements = 0;
  
  volatile bool on_ = false;

  float si(float x) { return sin(fract(x) * M_PI * 2.0); }
//  float sc(float x) { return clamp(si(x), -0.707, 0.707); }
  float sc(float x) { return clamp(si(x), -0.6, 0.6); }
  float buzz(float x) {
    x = fract(x) * 10.0;
    return sin(exp(2.5 - x) - 0.2);
  }
  float humm(float x) {
    return sc(x)*0.75 + si(x * 2.0)*0.75/2;
  }

  LightSaberSynth() :
    sin_sampler_a_hi_(sin_table),
    sin_sampler_a_lo_(sin_table),
    sin_sampler_b_(sin_table),
    buzz_sampler_(buzz_),
    humm_sampler_hi_(humm_),
    humm_sampler_lo_(humm_),
    volume_(32768 / 100) {
    sin_sampler_a_hi_.delta_ = 137 * 65536 / AUDIO_RATE;
    sin_sampler_a_lo_.delta_ = 1024 * 65536 / AUDIO_RATE;
    sin_sampler_b_.delta_ = 300 * 65536 / AUDIO_RATE;
    AdjustDelta(0.0);
    for (int i = 0; i < 1024; i++) {
      float f = i/1024.0;
//      sin_a_.table_[i] = 32768 / (3 + si(f));
//      sin_b_.table_[i] = 32766 / (3 + si(f));
      buzz_.table_[i] = 32766 * buzz(f);
      humm_.table_[i] = 32766 * humm(f);
    }
  }

  void AdjustDelta(float speed) {
    float cents = 1.0 - 0.5 * clamp(speed/200.0, -1.0, 1.0);
    float hz_to_delta = cents * 1024 * 65536 / AUDIO_RATE;
    buzz_sampler_.delta_ = 35 * hz_to_delta;
    humm_sampler_lo_.delta_ = 90 * hz_to_delta;
    humm_sampler_hi_.delta_ = 98 * hz_to_delta;
  }

  int read(int16_t *data, int elements) override {
    last_elements = elements;
    for (int i = 0; i < elements; i++) {
      int32_t tmp;
      tmp  = humm_sampler_lo_.next() *
	(32768 * 16383 / (3 * 16383 + sin_sampler_a_lo_.next()));
      tmp += humm_sampler_hi_.next() *
	(32768 * 16383 / (3 * 16383 + sin_sampler_a_hi_.next()));
      tmp += buzz_sampler_.next() *
	(32768 * 16383 / (3 * 16383 + sin_sampler_b_.next()));
      tmp >>= 15;
//      tmp = humm_sampler_lo_.next();
      last_prevolume_value = tmp;
      tmp = (tmp * (int32_t)volume_.value()) >> 15;
      volume_.advance();
      tmp = clampi32(tmp, -32768, 32767);
      last_value = tmp;
      data[i] = tmp;
    }
    return elements;
  }
protected:
  void Loop() override {
    if (monitor.ShouldPrint(Monitoring::MonitorSamples)) {
      Serial.print("Last elements: ");
      Serial.print(last_elements);
      Serial.print("Last sample: ");
      Serial.print(last_value);
      Serial.print(" prevol: ");
      Serial.print(last_prevolume_value);
      Serial.print(" vol: ");
      Serial.println(volume_.value());
    }
  }
};

// LightSaberSynth saber_synth;

#if 1

// Wanted audio capabilities:
// Background music (with independent volume)
// Options:
// 3 x EFFECT + synthesized HUM&SWING  (teensy style)
// 3 x EFFECT (wav) + { wav HUM or synthesized hum }  (nec style)
// HUM/EFFECTS spliced or x-faded (monophonic style)

// This means:
// 5 x WAV player (read from sdcard or serial flash) + HUM/SWING synthesizer
// WAV player needs gapless playback and find-cut support.

// Style #1
//  DAC
//   +-Dynamic Mixer
//   |  +-MonophonicFont (on/hum/off/swing/clash/etc.)
//   |     +-Buffer
//   |     |  +-WavPlayer
//   |     +-Buffer
//   |        +-WavPlayer
//   +-Buffer (for tracks)
//      +-WavPlayer
//
// Style #2 (superset of style #1)
//  DAC
//   +-Dynamic Mixer
//   |  +-MonophonicFont (on/hum/off)
//   |     +-Buffer
//   |     |  +-WavPlayer
//   |     +-Buffer
//   |        +-WavPlayer
//   +-Buffer (for tracks)
//   |   +-WavPlayer
//   +-Buffer (other sound fx)
//   |   +-WavPlayer
//   +-Buffer (other sound fx)
//   |   +-WavPlayer
//   +-Buffer (other sound fx)
//       +-WavPlayer
//

#define IRQ_WAV 55

class DataStreamWork;
DataStreamWork* data_streams;

class DataStreamWork {
public:
  DataStreamWork() {
    next_ = data_streams;
    data_streams = this;
    NVIC_SET_PRIORITY(IRQ_WAV, 240);
    _VectorsRam[IRQ_WAV + 16] = &ProcessDataStreams;
    NVIC_ENABLE_IRQ(IRQ_WAV);
  }
  ~DataStreamWork() {
    for (DataStreamWork** d = &data_streams; *d; d = &(*d)->next_) {
      if (*d == this) {
        *d = next_;
      }
    }
  }
  void scheduleFillBuffer() {
    if (!NVIC_IS_ACTIVE(IRQ_WAV))
      NVIC_TRIGGER_IRQ(IRQ_WAV);
  }

protected:
  virtual bool FillBuffer() = 0;

private:
  static void ProcessDataStreams() {
    for (int i = 0; i < 10; i++) {
      bool again = false;
      for (DataStreamWork *d = data_streams; d; d=d->next_) {
	if (d->FillBuffer()) {
	  again = true;
	}
      }
      if (!again) break;
    }
  }

  DataStreamWork* next_;
};

// N needs to be power of 2
template<class T, int N>
class BufferedDataStream : public DataStream<T>, public DataStreamWork {
public:
  BufferedDataStream() : DataStreamWork() {
  }
  int read(T* buf, int bufsize) override {
    int copied = 0;
    while (bufsize) {
      size_t to_copy = buffered();
      if (!to_copy) break;
      to_copy = min(to_copy, bufsize);
      size_t start_pos = buf_start_ & (N-1);
      to_copy = min(to_copy, N - start_pos);
      memcpy(buf, buffer_ + start_pos, sizeof(T) * to_copy);
      copied += to_copy;
      buf_start_ += to_copy;
      buf += to_copy;
      bufsize -= to_copy;
    }
    scheduleFillBuffer();
    return copied;
  }
  bool eof() override {
    return !buffered() && eof_;
  }
  void clear() {
    buf_start_ = buf_end_;
  }
  size_t buffered() const {
    return buf_end_ - buf_start_;
  }
  size_t space_available() const {
    return N - buffered();
  }
  void SetStream(DataStream<T>* stream) {
    eof_ = false;
    stream_ = stream;
  }
private:
  bool FillBuffer() override {
    if (stream_) {
      size_t space = space_available();
      if (space) {
	size_t end_pos = buf_end_ & (N-1);
	size_t to_read = min(space, N - end_pos);
	int got = stream_->read(buffer_ + end_pos, to_read);
	if (!got) eof_ = stream_->eof();
	buf_end_ += got;
      }
    }
    return stream_ && space_available() > 0 && !eof_;
  }
  DataStream<T> * volatile stream_ = 0;
  // Note, these are assumed to be atomic, 8-bit processors won't work.
  volatile size_t buf_start_ = 0;
  volatile size_t buf_end_ = 0;
  volatile bool eof_ = false;
  T buffer_[N];
};

#endif

#endif  // ENABLE_AUDIO

class Effect;
Effect* all_effects = NULL;

int constexpr toLower(char x) {
  return (x >= 'A' && x <= 'Z') ? x - 'A' + 'a' : x;
}

const char *startswith(const char *prefix, const char* x) {
  while (*prefix) {
    if (toLower(*x) != toLower(*prefix)) return nullptr;
    prefix++;
    x++;
  }
  return x;
}

int cmpdir(const char *a, const char *b) {
  while (toLower(*a) == toLower(*b)) {
    if (!*a) return 0;
    a++;
    b++;
  }
  if (*a == '/' && *b == 0) return 0;
  if (*b == '/' && *a == 0) return 0;
  return toLower(*a) - toLower(*b);
}

int parse1hex(const char* x) {
  int ret = toLower(*x);
  if (ret > 'a') return ret - 'a' + 10;
  return ret - '0';
}

int parse2hex(const char* x) {
  return (parse1hex(x) << 4) | parse1hex(x+1);
}

bool endswith(const char *postfix, const char* x) {
  size_t l = strlen(x);
  if (l < strlen(postfix)) return false;
  x = x + l - strlen(postfix);
  while (*postfix) {
    if (toLower(*x) != toLower(*postfix)) return false;
    postfix++;
    x++;
  }
  return true;
}

char current_directory[128];

// Effect represents a set of sound files.
class Effect {
  public:
  Effect(const char* name) : name_(name) {
    next_ = all_effects;
    all_effects = this;
    reset();
  }

  void reset() {
    min_file_ = 20000;
    max_file_ = -1;
    digits_ = 0;
    unnumbered_file_found_ = false;
    subdirs_ = false;
  }

  void Scan(const char *filename) {
    const char *rest = startswith(name_, filename);
    if (!rest) return;
    if (*rest == '/') {
      subdirs_ = true;
      const char *tmp = startswith(name_, rest + 1);
      if (!tmp) return;
      rest = tmp;
    }

    int n = -1;
    if (*rest == '.') {
      unnumbered_file_found_ = true;
    } else {
      char *end;
      n = strtol(rest, &end, 0);
      if (n <= 0) return;
      max_file_ = max(max_file_, n);
      min_file_ = min(min_file_, n);
      if (*rest == '0') {
        digits_ = end - rest;
      }
    }
  }

  void Show() {
    if (files_found()) {
      Serial.print("Found ");
      Serial.print(name_);
      Serial.print(" files: ");
      if (min_file_ <= max_file_) {
	Serial.print(min_file_);
	Serial.print("-");
	Serial.print(max_file_);
	if (digits_) {
	  Serial.print(" using ");
	  Serial.print(digits_);
	  Serial.print(" digits");
	}
	if (unnumbered_file_found_) {
	  Serial.print(" + ");
	}
      }
      if (unnumbered_file_found_) {
	Serial.print("one unnumbered file");
      }
      if (subdirs_) {
	Serial.print(" in subdirs");
      }
      Serial.println("");
    }
  }

  static void ShowAll() {
    for (Effect* e = all_effects; e; e = e->next_) {
      e->Show();
    }
    Serial.println("Done listing effects.");
  }

  size_t files_found() const {
    size_t ret = 0;
    if (min_file_ <= max_file_) {
      ret += max_file_ - min_file_ + 1;
    }
    if (unnumbered_file_found_) {
      ret ++;
    }
    return ret;
  }


  bool Play(char *filename) {
    int num_files = files_found();
    if (num_files < 1) return false;
    int n = rand() % num_files;
    strcpy(filename, current_directory);
    strcat(filename, name_);
    if (subdirs_) {
      strcat(filename, "/");
      strcat(filename, name_);
    }
    n += min_file_;
    // n can be max_file_ + 1, which means pick the file without digits.
    if (n <= max_file_) {
      char buf[12];
      itoa(n, buf, 10);
      char *j = filename + strlen(filename);
      int num_digits = strlen(buf);
      while (num_digits < digits_) {
        *j = '0';
        ++j;
        num_digits++;
      }
      memcpy(j, buf, strlen(buf) + 1);
    }

    strcat(filename, ".wav");
    Serial.print("Playing ");
    Serial.println(filename);
    return true;
  }

  static void ScanAll(const char* filename) {
    if (!endswith(".wav", filename))
      return;
#if 0
    // TODO: "monitor scan" command?
    Serial.print("SCAN ");
    Serial.println(filename);
#endif
    for (Effect* e = all_effects; e; e = e->next_) {
      e->Scan(filename);
    }
  }

  static void ScanDirectory(const char *directory) {
    Serial.print("Scanning sound font: ");
    Serial.println(directory);
    for (Effect* e = all_effects; e; e = e->next_) {
      e->reset();
    }

#ifdef ENABLE_SERIALFLASH
    // Scan serial flash.
    SerialFlashChip::opendir();
    uint32_t size;
    char filename[128];
    while (SerialFlashChip::readdir(filename, sizeof(filename), size)) {
      const char* f = startswith(directory, filename);
      if (f) ScanAll(f);
    }
#endif

#ifdef ENABLE_SD
    File dir = SD.open(directory);
    if (dir) {
      while (File f = dir.openNextFile()) {
	if (f.isDirectory()) {
	  char fname[128];
	  strcpy(fname, f.name());
	  strcat(fname, "/");
	  char* fend = fname + strlen(fname);
	  while (File f2 = f.openNextFile()) {
	    strcpy(fend, f2.name());
	    ScanAll(fname);
	    f2.close();
	  }
	} else {
	  ScanAll(f.name());
	}
	f.close();
      }
    }
#endif
  };

private:
  Effect* next_;

  // Minimum file number.
  int min_file_;

  // Maximum file number.
  int max_file_;

  // Leading zeroes are used to make it this many digits.
  int digits_;

  // If true. there is an un-numbered file as well.
  bool unnumbered_file_found_;

  // Files are in subdirectories, like "lock/lockNN.wav"
  bool subdirs_;

  // All files must start with this prefix.
  const char* name_;
};

#define EFFECT(X) Effect X(#X)

// Monophonic fonts
EFFECT(boot);
EFFECT(swing);
EFFECT(hum);
EFFECT(poweron);
EFFECT(poweroff);
EFFECT(pwroff);
EFFECT(clash);
EFFECT(force);
EFFECT(stab);
EFFECT(blaster);
EFFECT(lockup);
EFFECT(poweronf);
EFFECT(font);

// Polyphonic fonts
EFFECT(blst);
EFFECT(clsh);
EFFECT(in);
EFFECT(out);
EFFECT(lock);
EFFECT(swng);
EFFECT(slsh);

#ifdef ENABLE_AUDIO

// Simple upsampler code, doubles the number of samples with
// 2-lobe lanczos upsampling.
#define C1 24757
#define C2 -8191

#if 1
#define UPSAMPLE_FUNC(NAME, EMIT)                               \
  void NAME(int16_t sample) {                                   \
    upsample_buf_##NAME##_a_ = upsample_buf_##NAME##_b_;        \
    upsample_buf_##NAME##_b_ = upsample_buf_##NAME##_c_;        \
    upsample_buf_##NAME##_c_ = upsample_buf_##NAME##_d_;        \
    upsample_buf_##NAME##_d_ = sample;                          \
    EMIT(clampi32((upsample_buf_##NAME##_a_ * C2 +		\
          upsample_buf_##NAME##_b_ * C1 +                       \
          upsample_buf_##NAME##_c_ * C1 +                       \
		   upsample_buf_##NAME##_d_ * C2) >> 15,	\
		  -32768, 32767));				\
    EMIT(upsample_buf_##NAME##_c_);                             \
  }                                                             \
  void clear_##NAME() {                                         \
    upsample_buf_##NAME##_a_ = 0;                               \
    upsample_buf_##NAME##_b_ = 0;                               \
    upsample_buf_##NAME##_c_ = 0;                               \
    upsample_buf_##NAME##_d_ = 0;                               \
  }                                                             \
  int16_t upsample_buf_##NAME##_a_ = 0;                         \
  int16_t upsample_buf_##NAME##_b_ = 0;                         \
  int16_t upsample_buf_##NAME##_c_ = 0;                         \
  int16_t upsample_buf_##NAME##_d_ = 0
#else
#define UPSAMPLE_FUNC(NAME, EMIT)		\
  void NAME(int16_t sample) {			\
      EMIT(sample);      EMIT(sample);		\
  }						\
  void clear_##NAME() {				\
  }
#endif

#define DOWNSAMPLE_FUNC(NAME, EMIT)                     \
  void NAME(int16_t sample) {                           \
    if (downsample_flag_##NAME##_) {                    \
      EMIT((downsample_buf_##NAME##_ + sample) >> 1);   \
      downsample_flag_##NAME##_ = false;                \
    } else {                                            \
      downsample_buf_##NAME##_ = sample;                \
      downsample_flag_##NAME##_ = true;                 \
    }                                                   \
  }                                                     \
  void clear_##NAME() {                                 \
    downsample_buf_##NAME##_ = 0;                       \
    downsample_flag_##NAME##_ = false;                  \
  }                                                     \
  int16_t downsample_buf_##NAME##_ = 0;                 \
  bool downsample_flag_##NAME##_ = false

// PlayWav reads a file from serialflash or SD and converts
// it into a stream of samples. Note that because it can
// spend some time reading data between samples, the
// reader must have enough buffers to provide smooth playback.
class PlayWav : StateMachine, public DataStream<int16_t> {
public:
  void Play(const char* filename) {
    strcpy(filename_, filename);
    run_ = true;
  }

  void PlayOnce(Effect* effect) {
    if (effect->Play(filename_)) {
      effect_ = nullptr;
      run_ = true;
    }
  }
  void PlayLoop(Effect* effect) {
    effect_ = effect;
  }

  void Stop() {
    reset_state_machine();
    effect_ = nullptr;
    run_ = false;
  }

  bool isPlaying() const {
    return run_;
  }

private:
  void Emit1(uint16_t sample) {
    samples_[num_samples_++] = sample;
  }
  UPSAMPLE_FUNC(Emit2, Emit1);
  UPSAMPLE_FUNC(Emit4, Emit2);
  DOWNSAMPLE_FUNC(Emit05, Emit1);

  uint32_t header(int n) const {
    return ((uint32_t *)buffer)[n];
  }

  template<int bits> int16_t read2() {
    if (bits == 8) return *(ptr_++) << 8;
    return *((*((int16_t**)&ptr_))++);
  }

  template<int bits, int channels, int rate>
  void DecodeBytes4() {
    while (ptr_ < end_ && num_samples_ < NELEM(samples_)) {
      int v = 0;
      if (channels == 1) {
	v = read2<bits>();
      } else {
      	v = read2<bits>();
	v += read2<bits>();
	v >>= 1;
      }
      if (rate == AUDIO_RATE) {
        Emit1(v);
      } else if (rate == AUDIO_RATE / 2) {
        Emit2(v);
      } else if (rate == AUDIO_RATE / 4) {
        Emit4(v);
      } else if (rate == AUDIO_RATE * 2) {
        Emit05(v);
      } else {
	Serial.println("Unsupported rate.");
	Stop();
      }
    }
  }

  template<int bits, int channels>
  void DecodeBytes3() {
    if (rate_ == 44100)
      DecodeBytes4<bits, channels, 44100>();
    else if (rate_ == 22050)
      DecodeBytes4<bits, channels, 22050>();
    else if (rate_ == 11025)
      DecodeBytes4<bits, channels, 11025>();
  }

  template<int bits>
  void DecodeBytes2() {
    if (channels_ == 1) DecodeBytes3<bits, 1>();
    else DecodeBytes3<bits, 2>();
  }

  void DecodeBytes() {
    if (bits_ == 8) DecodeBytes2<8>();
    else DecodeBytes2<16>();
  }

  int ReadFile(int n) {
#ifdef ENABLE_SERIALFLASH
    if (sf_file_) {
      return sf_file_.read(buffer, n);
    }
#endif
#ifdef ENABLE_SD
    return sd_file_.read(buffer, n);
#else
    return 0;
#endif
  }

  void Skip(int n) {
#ifdef ENABLE_SERIALFLASH
    if (sf_file_) {
      sf_file_.seek(sf_file_.position() + n);
      return;
    }
#endif
#ifdef ENABLE_SD
    sd_file_.seek(sd_file_.position() + n);
#endif
  }

  int AlignRead(int n) {
#ifdef ENABLE_SERIALFLASH
    if (sf_file_) return n;
#endif
#ifdef ENABLE_SD
    int next_block = (sd_file_.position() + 512u) & ~511u;
    int bytes_to_end_of_block = next_block - sd_file_.position();
    return min(n, bytes_to_end_of_block);
#else
    return n;
#endif
  }

  void loop() {
    STATE_MACHINE_BEGIN();
    while (true) {
      while (!run_ && !effect_) YIELD();
      if (!run_) {
	if (!effect_->Play(filename_)) {
	  goto fail;
	}
        run_ = true;
      }
#ifdef ENABLE_SERIALFLASH
      sf_file_ = SerialFlashChip::open(filename_);
      if (!sf_file_)
#endif
      {
#ifdef ENABLE_SD
        sd_file_ = SD.open(filename_);
	YIELD();
        if (!sd_file_)
#endif
        {
	  Serial.print("File ");
	  Serial.print(filename_);
	  Serial.println(" not found.");
	  goto fail;
        }
      }
      if (ReadFile(20) != 20) {
	Serial.println("Failed to read 20 bytes.");
	goto fail;
      }
      if (header(0) != 0x46464952 &&
          header(2) != 0x45564157 &&
          header(3) != 0x20746D66 &&
          header(4) < 16) {
	Serial.println("Headers don't match.");
	YIELD();
	goto fail;
      }
      tmp_ = header(4);
      if (tmp_ != ReadFile(tmp_)) {
	Serial.println("Read failed.");
	goto fail;
      }
      if ((header(0) & 0xffff) != 1) {
	Serial.println("Wrong format.");
	goto fail;
      }
      channels_ = header(0) >> 16;
      rate_ = header(1);
      bits_ = header(3) >> 16;
      Serial.print("channels: ");
      Serial.print(channels_);
      Serial.print(" rate: ");
      Serial.print(rate_);
      Serial.print(" bits: ");
      Serial.println(bits_);

      while (ReadFile(8) == 8) {
        len_ = header(1);
        if (header(0) != 0x61746164) {
          Skip(len_);
          continue;
        }
	sample_bytes_ = len_;
        while (len_) {
	  bytes_to_decode_ =
	    ReadFile(AlignRead(min(len_, sizeof(buffer))));
	  len_ -= bytes_to_decode_;
	  ptr_ = buffer;
	  end_ = buffer + bytes_to_decode_;
          while (ptr_ < end_) {
	    DecodeBytes();

            while (written_ < num_samples_) {
              // Preload should go to here...
              while (to_read_ == 0) YIELD();

              int n = min(num_samples_ - written_, to_read_);
              memcpy(dest_, samples_ + written_, n * 2);
              dest_ += n;
              written_ += n;
              to_read_ -= n;
            }
            written_ = num_samples_ = 0;
          }
        }
	YIELD();
      }

      // EOF;
      run_ = false;
      continue;

  fail:
      run_ = false;
      YIELD();
    }

    STATE_MACHINE_END();
  }

public:
  // Called from interrupt handler.
  int read(int16_t* dest, int to_read) override {
    dest_ = dest;
    to_read_ = to_read;
    loop();
    return dest_ - dest;
  }

  bool eof() override {
    return !run_;
  }

  // Length, seconds.
  float length() const {
    return (float)(sample_bytes_) * 8 / (bits_ * rate_);
  }

private:
  volatile bool run_ = false;
  Effect* volatile effect_ = nullptr;
  char filename_[128];
#ifdef ENABLE_SD
  File sd_file_;
#endif
#ifdef ENABLE_SERIALFLASH
  SerialFlashFile sf_file_;
#endif
  int16_t* dest_ = nullptr;
  int to_read_ = 0;
  int tmp_;

  int rate_;
  int channels_;
  int bits_;

  int bytes_to_decode_ = 0;
  size_t len_ = 0;
  volatile size_t sample_bytes_ = 0;
  char* ptr_;
  char* end_;
  char buffer[512]  __attribute__((aligned(4)));

  // Number of samples_ in samples that has been
  // sent out already.
  int written_ = 0;
  
  // Number of samples in samples_
  int num_samples_ = 0;
  int16_t samples_[32];
};


// Combines a WavPlayer and a BufferedDataStream into a
// buffered wav player. When we start a new sample, we
// make sure to fill up the buffer before we start playing it.
// This minimizes latency while making sure to avoid any gaps.
class BufferedWavPlayer : public DataStream<int16_t> {
public:
  void Play(const char* filename) {
    pause_ = true;
    buffer.clear();
    wav.Play(filename);
    buffer.scheduleFillBuffer();
    pause_ = false;
  }

  void PlayOnce(Effect* effect) {
    pause_ = true;
    buffer.clear();
    wav.PlayOnce(effect);
    buffer.scheduleFillBuffer();
    pause_ = false;
  }
  void PlayLoop(Effect* effect) { wav.PlayLoop(effect); }
  void Stop() {
    wav.Stop();
    pause_ = true;
    buffer.clear();
  }

  bool isPlaying() const {
    return wav.isPlaying();
  }

  BufferedWavPlayer() {
    buffer.SetStream(&wav);
  }

  int read(int16_t* dest, int to_read) override {
    if (pause_) return 0;
    return buffer.read(dest, to_read);
  }

  float length() const { return wav.length(); }

private:
  BufferedDataStream<int16_t, 512> buffer;
  PlayWav wav;
  volatile bool pause_;
};

BufferedWavPlayer wav_players[4];

// This class is used to cut from one sound to another with
// no gap. It does a short (2.5ms) crossfade to accomplish this.
class AudioSplicer : public DataStream<int16_t> {
public:
  AudioSplicer() : volume_(16384 / 100) {
    set_volume(10000);
  }
  int read(int16_t* data, int elements) override {
    int16_t *p = data;
    int to_read = elements;
    if (current_ < 0) {
      if (start_after_) {
	start_after_ -= elements;
	if (start_after_ < 0) {
	  start_after_ = 0;
	  current_ = fadeto_;
	  fadeto_ = -1;
	}
      }
      return 0;
    }

    int num = players_[current_].read(p, to_read);
    to_read -= num;
    p += num;
    if (num < to_read) {
      // end of file.
      current_ = -1;
      fadeto_ = -1;
      while (num < to_read) {
	*(p++) = 0;
	to_read --;
      }
    }
    if (fadeto_ >= 0) {
      p = data;
      to_read = elements;
      while (to_read) {
        int16_t tmp[32];
        int n = min(to_read, (int)NELEM(tmp));
        int num = players_[fadeto_].read(tmp, n);
        while (num < n) tmp[num++] = 0;
        for (int i = 0; i < num; i++) {
          p[i] = (p[i] * fade_ + tmp[i] * (32768 - fade_)) >> 15;
          if (fade_) {
	    fade_ -= fade_speed_;
	    if (fade_ < 0) fade_ = 0;
	  }
        }
        to_read -= n;
	p += n;
      }
      if (!fade_) {
	players_[current_].Stop();
        current_ = fadeto_;
        fadeto_ = -1;
      }
    }
    for (int i = 0; i < elements; i++) {
      int32_t v = (data[i] * (int32_t)volume_.value()) >> 15;
      data[i] = clampi32(v, -32768, 32767);
      volume_.advance();
    }
    return elements;
  }

  bool eof() override {
    return current_ == -1;
  }

  void set_fade_time(float t) {
    fade_speed_ = max(1, (int)(32768 / t / AUDIO_RATE));
    Serial.print("FADE SPEED: ");
    Serial.println(fade_speed_);
  }

  bool Play(Effect* f, Effect* loop, int delay_ms = 0) {
    if (fadeto_ != -1) {
      Serial.print("cutover unit busy fade_ =");
      Serial.print(fade_);
      Serial.print(" fadeto_ = ");
      Serial.print(fadeto_);
      Serial.print(" current_ = ");
      Serial.println(current_);
      // Need to finish fading to the previous unit first. (~2.5ms)
      // Perhaps we should just wait for it?
      return false;
    }
    digitalWrite(amplifierPin, HIGH); // turn on the amplifier
    int unit = current_ == 0 ? 1 : 0;
    players_[unit].PlayOnce(f);
    if (loop) {
      players_[unit].PlayLoop(loop);
    }
    if (delay_ms) {
      fadeto_ = unit;
      start_after_ = delay_ms * (AUDIO_RATE/100) / 10;
#if 0
      Serial.print("START AFTER: ");
      Serial.println(start_after_);
#endif
    } else {
      if (current_ == -1) {
	current_ = unit;
      } else {
	fadeto_ = unit;
	fade_ = 32768;
      }
    }
    return true;
  }

  bool isPlaying() const {
    return current_ != -1;
  }

  void set_volume(int vol) {
    volume_.set_target(vol);
  }

  void Stop() {
    players_[0].Stop();
    players_[1].Stop();
  }

protected:
  BufferedWavPlayer players_[2];
  volatile int current_= -1;
  volatile int fadeto_ = -1;
  volatile int fade_speed_ = 128;
  volatile int start_after_ = 0;
  volatile int fade_;
  ClickAvoiderLin volume_;
};

AudioSplicer audio_splicer;

class MonophonicFont : SaberBase {
public:
  MonophonicFont() : SaberBase(NOLINK) { }
  void Activate() {
    Serial.println("Activating monophonic font.");
    audio_splicer.set_fade_time(0.003);
    SaberBase::Link();
  }
  void Deactivate() { SaberBase::Unlink(); }

  void On() override {
    audio_splicer.Play(&poweron, &hum);
  }

  void Off() override {
    audio_splicer.Play(&poweroff, NULL);
  }
  void Clash() override { audio_splicer.Play(&clash, &hum); }
  void Stab() override { audio_splicer.Play(&stab, &hum); }
  void Force() override { audio_splicer.Play(&force, &hum); }
  void Blast() override { audio_splicer.Play(&blaster, &hum); }
  void Boot() override { audio_splicer.Play(&boot,  NULL); }
  void NewFont() override { audio_splicer.Play(&font,  NULL); }

  bool swinging_ = false;
  void Motion(float speed) override {
    if (speed > 250.0) {
      if (!swinging_) {
	swinging_ = true;
	audio_splicer.Play(&swing, &hum);
      }
    } else {
      swinging_ = false;
    }
    int vol = 10000;
    if (!swinging_) {
      vol = vol * (0.99 + clamp(speed/200.0, 0.0, 1.0));
    }
    audio_splicer.set_volume(vol);
  }
  void XMotion(float speed) override {}
};

MonophonicFont monophonic_font;

struct ConfigFile {
#ifdef ENABLE_SD
  void skipwhite(File* f) {
    while (f->peek() == ' ' || f->peek() == '\t')
      f->read();
  }
  void skipline(File* f) {
    while (f->available() && f->read() != '\n');
  }

  int64_t readValue(File* f) {
    int64_t ret = 0;
    int64_t sign = 1;
    if (f->peek() == '-') {
      sign = -1;
      f->read();
    }
    while (f->available()) {
      int c = toLower(f->peek());
      if (c >= '0' && c <= '9') {
	ret = (c - '0') + 10 * ret;
	f->read();
      } else {
	return ret * sign;
      }
    }
    return ret * sign;
  }

  void Read(File* f) {
    for (; f->available(); skipline(f)) {
      char variable[33];
      variable[0] = 0;
      skipwhite(f);
      if (f->peek() == '#') continue;
      for (int i = 0; i < 32; i++) {
	int c = toLower(f->peek());
	if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
	  f->read();
	  variable[i] = c;
	  variable[i+1] = 0;
	} else {
	  break;
	}
      }
      skipwhite(f);
      if (f->peek() != '=') continue;
      f->read();
      skipwhite(f);
      int64_t v = readValue(f);
#if 0
      Serial.print(variable);
      Serial.print(" = ");
      Serial.println((int)v);
#endif
      if (!strcmp(variable, "humstart")) {
	humStart = v;
      }
    }
  }
#endif

  void Read(const char *filename) {
#ifdef ENABLE_SD
    File f = SD.open(filename);
    Read(&f);
#endif
  }


  int humStart = 100;
};

class PolyphonicFont : public SaberBase {
public:
  PolyphonicFont() : SaberBase(NOLINK) { }
  void Activate() {
    Serial.println("Activating polyphonic font.");
    // TODO: while we need to fade out the hum,
    // maybe we don't need to fade in the in sound?
    audio_splicer.set_fade_time(0.3);
    char config_filename[128];
    strcpy(config_filename, current_directory);
    strcat(config_filename, "config.ini");
    config_.Read(config_filename);
    SaberBase::Link();
  }
  void Deactivate() { SaberBase::Unlink(); }

  void On() override {
    if (config_.humStart) {
      BufferedWavPlayer* tmp = Play(&out);
      if (tmp) {
	int delay_ms = 1000 * tmp->length() - config_.humStart;
#if 1
	Serial.print(" LEN = ");
	Serial.print(tmp->length());
	Serial.print(" humstart = ");
	Serial.print(config_.humStart);
	Serial.print(" delay_ms = ");
	Serial.println(delay_ms);
#endif
	audio_splicer.Play(&hum, &hum, delay_ms);
	return;
      }
    }
    audio_splicer.Play(&out, &hum);
  }

  void Off() override {
    audio_splicer.Play(&in, NULL);
  }

  BufferedWavPlayer* Play(Effect* f)  {
    digitalWrite(amplifierPin, HIGH); // turn on the amplifier
    // Find a free wave playback unit.
    for (size_t unit = 0; unit < NELEM(wav_players); unit++) {
      if (!wav_players[unit].isPlaying()) {
	wav_players[unit].PlayOnce(f);
	return wav_players + unit;
      }
    }
    return NULL;
  }
  void Clash() override { Play(&clsh); }
  void Stab() override { Play(&stab); }
  void Force() override { Play(&force); }
  void Blast() override { Play(&blst); }
  void Boot() override { audio_splicer.Play(&boot,  NULL); }
  void NewFont() override { audio_splicer.Play(&font,  NULL); }

  bool swinging_ = false;
  void Motion(float speed) override {
    if (speed > 250.0) {
      if (!swinging_) {
	swinging_ = true;
	Play(&swng);
      }
    } else {
      swinging_ = false;
    }
    int vol = 10000;
    if (!swinging_) {
      vol = vol * (0.99 + clamp(speed/200.0, 0.0, 1.0));
    }
    audio_splicer.set_volume(vol);
  }
  void XMotion(float speed) override {}

  ConfigFile config_;
};

PolyphonicFont polyphonic_font;

class SyntheticFont : PolyphonicFont {
public:
  void On() override {}
  void Off() override {}

  void Motion(float speed) override {
    // Adjust hum volume based on motion speed
  }
};
#endif  // ENABLE_AUDIO

class BatteryMonitor : Looper, CommandParser {
public:
  float battery_now() {
    float volts = 3.3 * analogRead(batteryLevelPin) / 1024.0;
#if VERSION_MAJOR >= 2
    float pulldown = 220000;  // External pulldown
    float pullup = 2000000;  // External pullup
#else
    float pulldown = 33000;  // Internal pulldown is 33kOhm
    float pullup = 23000;  // External pullup
#endif
    float battery_volts = volts * (1.0 + pullup / pulldown);
    return battery_volts;
  }
  float battery() const {
    return last_voltage_;
  }
  
  bool low() const {
    return battery() < 3.0;
  }
protected:
  void Setup() override {
    really_old_voltage_ = old_voltage_ = last_voltage_ = battery_now();
#if VERSION_MAJOR >= 2
    pinMode(batteryLevelPin, INPUT);
#else
    pinMode(batteryLevelPin, INPUT_PULLDOWN);
#endif
  }
  void Loop() override {
    float v = battery_now();
    last_voltage_ = last_voltage_ * 0.999 + v * 0.001;
    if (monitor.ShouldPrint(Monitoring::MonitorBattery) ||
	millis() - last_print_millis_ > 20000) {
      Serial.print("Battery voltage: ");
      Serial.println(battery());
      last_print_millis_ = millis();
    }
  }

  bool Parse(const char* cmd, const char* arg) override {
    if (!strcmp(cmd, "batt") || !strcmp(cmd, "battery")) {
      Serial.print("Battery voltage: ");
      Serial.println(battery());
      return true;
    }
    return false;
  }
private:
  float last_voltage_ = 0.0;
  float old_voltage_ = 0.0;
  float really_old_voltage_ = 0.0;
  uint32_t last_print_millis_;
  uint32_t last_beep_ = 0;
};

BatteryMonitor battery_monitor;

struct Color {
  Color() : r(0), g(0), b(0) {}
  Color(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
  // x = 0..256
  Color mix(const Color& other, int x) const {
    // Wonder if there is an instruction for this?
    return Color( ((256-x) * r + x * other.r) >> 8,
                  ((256-x) * g + x * other.g) >> 8,
                  ((256-x) * b + x * other.b) >> 8);
  }
  uint8_t select(const Color& other) const {
    uint8_t ret = 255;
    if (other.r) ret = min(ret, r * 255 / other.r);
    if (other.g) ret = min(ret, g * 255 / other.g);
    if (other.b) ret = min(ret, b * 255 / other.b);
    return ret;
  }
  uint8_t r, g, b;
};

#ifdef ENABLE_WS2811
// What follows is a copy of the OctoWS2811 library. It's been modified in 
// the following ways:
//
//   1) It now only outputs data to one pin. (Which pin can be selected by
//      changing the "ones" bitfield below.
//   2) It has been modified to use the FTM timer to drive the DMA. This
//      frees up a few pins, but locks PWM frequencies for many pins to 800kHz
//   3) Number of LEDs and configuration is determined when you call begin()
//      instead of in the constructor to make different blade configurations possible.


/*  OctoWS2811 - High Performance WS2811 LED Display Library
    http://www.pjrc.com/teensy/td_libs_OctoWS2811.html
    Copyright (c) 2013 Paul Stoffregen, PJRC.COM, LLC

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#include <Arduino.h>
#include "DMAChannel.h"

#if TEENSYDUINO < 121
#error "Teensyduino version 1.21 or later is required to compile this library."
#endif
#ifdef __AVR__
#error "MonopodWS2811 does not work with Teensy 2.0 or Teensy++ 2.0."
#endif

#define WS2811_RGB      0       // The WS2811 datasheet documents this way
#define WS2811_RBG      1
#define WS2811_GRB      2       // Most LED strips are wired this way
#define WS2811_GBR      3

#define WS2811_800kHz 0x00      // Nearly all WS2811 are 800 kHz
#define WS2811_400kHz 0x10      // Adafruit's Flora Pixels
#define WS2811_580kHz 0x20      // PL9823

class MonopodWS2811 {
public:
  void begin(uint32_t numPerStrip,
             void *frameBuf,
             uint8_t config = WS2811_GRB);
  void setPixel(uint32_t num, Color color) {
    drawBuffer[num] = color;
  }
  Color getPixel(uint32_t num) {
    return drawBuffer[num];
  }

  void show(void);
  int busy(void);

  int numPixels(void) {
    return stripLen;
  }
  
private:
  static uint16_t stripLen;
  static void *frameBuffer;
  static Color drawBuffer[maxLedsPerStrip];
  static uint8_t params;
  static DMAChannel dma1, dma2, dma3;
  static void isr(void);
};

uint16_t MonopodWS2811::stripLen;
void * MonopodWS2811::frameBuffer;
uint8_t MonopodWS2811::params;
DMAChannel MonopodWS2811::dma1;
DMAChannel MonopodWS2811::dma2;
DMAChannel MonopodWS2811::dma3;
Color MonopodWS2811::drawBuffer[maxLedsPerStrip];
static uint8_t ones = 0x20;  // pin 20
static volatile uint8_t update_in_progress = 0;
static uint32_t update_completed_at = 0;


// Waveform timing: these set the high time for a 0 and 1 bit, as a fraction of
// the total 800 kHz or 400 kHz clock cycle.  The scale is 0 to 255.  The Worldsemi
// datasheet seems T1H should be 600 ns of a 1250 ns cycle, or 48%.  That may
// erroneous information?  Other sources reason the chip actually samples the
// line close to the center of each bit time, so T1H should be 80% if TOH is 20%.
// The chips appear to work based on a simple one-shot delay triggered by the
// rising edge.  At least 1 chip tested retransmits 0 as a 330 ns pulse (26%) and
// a 1 as a 660 ns pulse (53%).  Perhaps it's actually sampling near 500 ns?
// There doesn't seem to be any advantage to making T1H less, as long as there
// is sufficient low time before the end of the cycle, so the next rising edge
// can be detected.  T0H has been lengthened slightly, because the pulse can
// narrow if the DMA controller has extra latency during bus arbitration.  If you
// have an insight about tuning these parameters AND you have actually tested on
// real LED strips, please contact paul@pjrc.com.  Please do not email based only
// on reading the datasheets and purely theoretical analysis.
#define WS2811_TIMING_T0H  60
#define WS2811_TIMING_T1H  176

// Discussion about timing and flicker & color shift issues:
// http://forum.pjrc.com/threads/23877-WS2812B-compatible-with-OctoWS2811-library?p=38190&viewfull=1#post38190

static uint8_t analog_write_res = 8;

// TODO: Try replacing with FTM0 with FTM1 to unlock PWM frequencies for most pins.
void setFTM_Timer(uint8_t ch1, uint8_t ch2, float frequency)
{
  uint32_t prescale, mod, ftmClock, ftmClockSource;
  float minfreq;

  if (frequency < (float)(F_BUS >> 7) / 65536.0f) {     //If frequency is too low for working with F_TIMER:
    ftmClockSource = 2;                 //Use alternative 31250Hz clock source
    ftmClock = 31250;                   //Set variable for the actual timer clock frequency
  } else {                                                //Else do as before:
    ftmClockSource = 1;                 //Use default F_Timer clock source
    ftmClock = F_BUS;                    //Set variable for the actual timer clock frequency
  }

  for (prescale = 0; prescale < 7; prescale++) {
    minfreq = (float)(ftmClock >> prescale) / 65536.0f;    //Use ftmClock instead of F_TIMER
    if (frequency >= minfreq) break;
  }

  mod = (float)(ftmClock >> prescale) / frequency - 0.5f;    //Use ftmClock instead of F_TIMER
  if (mod > 65535) mod = 65535;

  FTM0_SC = 0; // stop FTM until setting of registers are ready
  FTM0_CNTIN = 0; // initial value for counter. CNT will be set to this value, if any value is written to FTMx_CNT
  FTM0_CNT = 0;
  FTM0_MOD = mod;

  // I don't know why, but the following code leads to a very short first pulse. Shifting the compare values to the end looks much better
  // uint32_t cval;
  // FTM0_C0V = 1;  // 0 is not working -> add 1 to every compare value.
  // cval = ((uint32_t)ch1 * (uint32_t)(mod + 1)) >> analog_write_res;
  // FTM0_C1V = cval +1;
  // cval = ((uint32_t)ch2 * (uint32_t)(mod + 1)) >> analog_write_res;
  // FTM0_C2V = cval +1;

  // Shifting the compare values to the end leads to a perfect first (and last) pulse:
  uint32_t cval1 = ((uint32_t)ch1 * (uint32_t)(mod + 1)) >> analog_write_res;
  uint32_t cval2 = ((uint32_t)ch2 * (uint32_t)(mod + 1)) >> analog_write_res;
  FTM0_C0V = mod - (cval2 - 0);
  FTM0_C1V = mod - (cval2 - cval1);
  FTM0_C2V = mod;

  FTM0_C0SC = FTM_CSC_DMA | FTM_CSC_CHIE | 0x28;
  FTM0_C1SC = FTM_CSC_DMA | FTM_CSC_CHIE | 0x28;
  FTM0_C2SC = FTM_CSC_DMA | FTM_CSC_CHIE | 0x28;

  FTM0_SC = FTM_SC_CLKS(ftmClockSource) | FTM_SC_PS(prescale);    //Use ftmClockSource instead of 1. Start FTM-Timer.
  //with 96MHz Teensy: prescale 0, mod 59, ftmClockSource 1, cval1 14, cval2 41
}

#ifdef INVERT_WS2811
#define WS2811_PORT_CLEAR GPIOD_PSOR
#define WS2811_PORT_SET   GPIOD_PCOR
#else
#define WS2811_PORT_CLEAR GPIOD_PCOR
#define WS2811_PORT_SET   GPIOD_PSOR
#endif

void MonopodWS2811::begin(uint32_t numPerStrip,
                          void *frameBuf,
                          uint8_t config)
{
  stripLen = numPerStrip;
  frameBuffer = frameBuf;
  params = config;

  uint32_t bufsize, frequency = 400000;

  bufsize = stripLen*24;

  // set up the buffers
  memset(frameBuffer, ones, bufsize);
        
  // configure the 8 output pins
  WS2811_PORT_CLEAR = ones;
  if (ones & 1)   pinMode(2, OUTPUT);   // strip #1
  if (ones & 2)   pinMode(14, OUTPUT);  // strip #2
  if (ones & 4)   pinMode(7, OUTPUT);   // strip #3
  if (ones & 8)   pinMode(8, OUTPUT);   // strip #4
  if (ones & 16)  pinMode(6, OUTPUT);   // strip #5
  if (ones & 32)  pinMode(20, OUTPUT);  // strip #6
  if (ones & 64)  pinMode(21, OUTPUT);  // strip #7
  if (ones & 128) pinMode(5, OUTPUT);   // strip #8

  int t0h = WS2811_TIMING_T0H;
  int t1h = WS2811_TIMING_T1H;
  switch (params & 0xF0) {
    case WS2811_400kHz:
      frequency = 400000;
      break;

    case WS2811_800kHz:
      frequency = 740000;
      break;

    case WS2811_580kHz:
      frequency = 580000;
      break;
  }

#ifdef USE_FTM_TIMER
  setFTM_Timer(t0h, t1h, frequency);
#else  // USE_FTM_TIMER
  // create the two waveforms for WS2811 low and high bits
  // FOOPIN Must be 4 or 17, and it can't be 4....
  // Need help changing 4 to 17...
#define FOOPIN 4
#define BARPIN 3
#define FOOCAT32(X,Y,Z) X##Y##Z
#define FOOCAT3(X,Y,Z) FOOCAT32(X,Y,Z)

  analogWriteResolution(8);
  analogWriteFrequency(BARPIN, frequency);
  analogWriteFrequency(FOOPIN, frequency);
  analogWrite(BARPIN, t0h);
  analogWrite(FOOPIN, t1h);

#if defined(KINETISK)
  // pin 16 triggers DMA(port B) on rising edge (configure for pin 3's waveform)
  CORE_PIN16_CONFIG = PORT_PCR_IRQC(1)|PORT_PCR_MUX(3);
  pinMode(BARPIN, INPUT_PULLUP); // pin 3 no longer needed

  // pin 15 triggers DMA(port C) on falling edge of low duty waveform
  // pin 15 and 16 must be connected by the user: 16 is output, 15 is input
  pinMode(15, INPUT);
  CORE_PIN15_CONFIG = PORT_PCR_IRQC(2)|PORT_PCR_MUX(1);

  // pin FOOPIN triggers DMA(port A) on falling edge of high duty waveform
  FOOCAT3(CORE_PIN,FOOPIN,_CONFIG) = PORT_PCR_IRQC(2)|PORT_PCR_MUX(3);

#elif defined(KINETISL)
  // on Teensy-LC, use timer DMA, not pin DMA
  //Serial1.println(FTM2_C0SC, HEX);
  //FTM2_C0SC = 0xA9;
  //FTM2_C0SC = 0xA9;
  //uint32_t t = FTM2_C0SC;
  //FTM2_C0SC = 0xA9;
  //Serial1.println(t, HEX);
  CORE_PIN3_CONFIG = 0;
  FOOCAT3(CORE_PIN,FOOPIN,_CONFIG) = 0;
  //FTM2_C0SC = 0;
  //FTM2_C1SC = 0;
  //while (FTM2_C0SC) ;
  //while (FTM2_C1SC) ;
  //FTM2_C0SC = 0x99;
  //FTM2_C1SC = 0x99;

  //MCM_PLACR |= MCM_PLACR_ARB;

#endif

#endif  // USE_FTM_TIMER

  // DMA channel #1 sets WS2811 high at the beginning of each cycle
  dma1.source(ones);
  dma1.destination(WS2811_PORT_SET);
  dma1.transferSize(1);
  dma1.transferCount(bufsize);
  dma1.disableOnCompletion();

  // DMA channel #2 writes the pixel data at 20% of the cycle
  dma2.sourceBuffer((uint8_t *)frameBuffer, bufsize);
  dma2.destination(WS2811_PORT_CLEAR);
  dma2.transferSize(1);
  dma2.transferCount(bufsize);
  dma2.disableOnCompletion();

  // DMA channel #3 clear all the pins low at 48% of the cycle
  dma3.source(ones);
  dma3.destination(WS2811_PORT_CLEAR);
  dma3.transferSize(1);
  dma3.transferCount(bufsize);
  dma3.disableOnCompletion();
  dma3.interruptAtCompletion();

#ifdef __MK20DX256__
  MCM_CR = MCM_CR_SRAMLAP(1) | MCM_CR_SRAMUAP(0);
  AXBS_PRS0 = 0x1032;
#endif

#ifdef USE_FTM_TIMER
  dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM0_CH0);
  dma2.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM0_CH1);
  dma3.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM0_CH2);
#elif defined(KINETISK)
  // route the edge detect interrupts to trigger the 3 channels
  dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_PORTB);
  dma2.triggerAtHardwareEvent(DMAMUX_SOURCE_PORTC);
  dma3.triggerAtHardwareEvent(DMAMUX_SOURCE_PORTA);
#elif defined(KINETISL)
  // route the timer interrupts to trigger the 3 channels
  dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_OV);
  dma2.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_CH0);
  dma3.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_CH1);
#endif

  // enable a done interrupts when channel #3 completes
  dma3.attachInterrupt(isr);
  //pinMode(9, OUTPUT); // testing: oscilloscope trigger
}

void MonopodWS2811::isr(void)
{
  //Serial1.print(".");
  //Serial1.println(dma3.CFG->DCR, HEX);
  //Serial1.print(dma3.CFG->DSR_BCR > 24, HEX);
  dma3.clearInterrupt();
  //Serial1.print("*");
  update_completed_at = micros();
  update_in_progress = 0;
}

int MonopodWS2811::busy(void)
{
  if (update_in_progress) return 1;
  // busy for 50 us after the done interrupt, for WS2811 reset
  if (micros() - update_completed_at < 50) return 1;
  return 0;
}

#if 1
static inline void Out2DMA(uint8_t *& o, uint8_t v) {
  *(o++) = (v & 128) ? 0 : ones;
  *(o++) = (v & 64) ? 0 : ones;
  *(o++) = (v & 32) ? 0 : ones;
  *(o++) = (v & 16) ? 0 : ones;
  *(o++) = (v & 8) ? 0 : ones;
  *(o++) = (v & 4) ? 0 : ones;
  *(o++) = (v & 2) ? 0 : ones;
  *(o++) = (v & 1) ? 0 : ones;
}

template<int STYLE>
void CopyOut(struct Color* inbuf, void* frameBuffer, int num) {
  uint8_t *o = (uint8_t*)frameBuffer;
  for (int j = 0; j < num; j++) {
    Color tmp = inbuf[j];
    if (STYLE == WS2811_RBG) {
      Out2DMA(o, tmp.r);
      Out2DMA(o, tmp.b);
      Out2DMA(o, tmp.g);
    } else if (STYLE == WS2811_GRB) {
      Out2DMA(o, tmp.g);
      Out2DMA(o, tmp.r);
      Out2DMA(o, tmp.b);
    } else if (STYLE == WS2811_GBR) {
      Out2DMA(o, tmp.g);
      Out2DMA(o, tmp.b);
      Out2DMA(o, tmp.r);
    } else {
      Out2DMA(o, tmp.r);
      Out2DMA(o, tmp.g);
      Out2DMA(o, tmp.b);
    }
  }
}
#endif

void MonopodWS2811::show(void)
{
  // wait for any prior DMA operation
  //Serial1.print("1");
  while (update_in_progress) ; 
  //Serial1.print("2");

  // it's ok to copy the drawing buffer to the frame buffer
  // during the 50us WS2811 reset time
  switch (params & 7) {
    case WS2811_RBG:
      CopyOut<WS2811_RBG>(drawBuffer, frameBuffer, stripLen);
      break;
    case WS2811_GRB:
      CopyOut<WS2811_GRB>(drawBuffer, frameBuffer, stripLen);
      break;
    case WS2811_GBR:
      CopyOut<WS2811_GBR>(drawBuffer, frameBuffer, stripLen);
      break;
    default:
      CopyOut<0>(drawBuffer, frameBuffer, stripLen);
      break;
  }

  // wait for WS2811 reset
  while (micros() - update_completed_at < 50) ;

#ifdef USE_FTM_TIMER
  uint32_t sc = FTM0_SC;
  //digitalWriteFast(1, HIGH); // oscilloscope trigger
  
  //noInterrupts(); // This code is not time critical anymore. IRQs can stay on. 
  // We disable the FTM Timer, reset it to its initial counter value and clear all irq-flags. 
  // Clearing irqs is a bit tricky, because with DMA enabled, only the DMA can clear them. 
  // We have to disable DMA, reset the irq-flags and enable DMA once again.
  update_in_progress = 1;
  FTM0_SC = sc & 0xE7;    // stop FTM timer
        
  FTM0_CNT = 0; // writing any value to CNT-register will load the CNTIN value!
        
  FTM0_C0SC = 0; // disable DMA transfer. It has to be done, because we can't reset the CHnF bit while DMA is enabled
  FTM0_C1SC = 0;
  FTM0_C2SC = 0;
        
  FTM0_STATUS; // read status and write 0x00 to it, clears all pending IRQs
  FTM0_STATUS = 0x00;
        
  FTM0_C0SC = FTM_CSC_DMA | FTM_CSC_CHIE | 0x28; 
  FTM0_C1SC = FTM_CSC_DMA | FTM_CSC_CHIE | 0x28;
  FTM0_C2SC = FTM_CSC_DMA | FTM_CSC_CHIE | 0x28;
        
  dma1.enable();
  dma2.enable();        // enable all 3 DMA channels
  dma3.enable();
  //interrupts();
  //digitalWriteFast(1, LOW);
  // wait for WS2811 reset
  while (micros() - update_completed_at < 50) ; // moved to the end, because everything else can be done before.
  FTM0_SC = sc;        // restart FTM timer

#elif defined(KINETISK)
  // ok to start, but we must be very careful to begin
  // without any prior 3 x 800kHz DMA requests pending
  uint32_t sc = FTM1_SC;
  uint32_t cv = FTM1_C1V;
  noInterrupts();
  // CAUTION: this code is timing critical.  Any editing should be
  // tested by verifying the oscilloscope trigger pulse at the end
  // always occurs while both waveforms are still low.  Simply
  // counting CPU cycles does not take into account other complex
  // factors, like flash cache misses and bus arbitration from USB
  // or other DMA.  Testing should be done with the oscilloscope
  // display set at infinite persistence and a variety of other I/O
  // performed to create realistic bus usage.  Even then, you really
  // should not mess with this timing critical code!
  update_in_progress = 1;
  while (FTM1_CNT <= cv) ; 
  while (FTM1_CNT > cv) ; // wait for beginning of an 800 kHz cycle
  while (FTM1_CNT < cv) ;
  FTM1_SC = sc & 0xE7;    // stop FTM1 timer (hopefully before it rolls over)
  //digitalWriteFast(9, HIGH); // oscilloscope trigger
  PORTB_ISFR = (1<<0);    // clear any prior rising edge
  PORTC_ISFR = (1<<0);    // clear any prior low duty falling edge
  PORTA_ISFR = (1<<13);   // clear any prior high duty falling edge
  dma1.enable();
  dma2.enable();          // enable all 3 DMA channels
  dma3.enable();
  FTM1_SC = sc;           // restart FTM1 timer
  //digitalWriteFast(9, LOW);
#elif defined(KINETISL)
  uint32_t sc = FTM2_SC;
  uint32_t cv = FTM2_C1V;
  noInterrupts();
  update_in_progress = 1;
  while (FTM2_CNT <= cv) ;
  while (FTM2_CNT > cv) ; // wait for beginning of an 800 kHz cycle
  while (FTM2_CNT < cv) ;
  FTM2_SC = 0;            // stop FTM2 timer (hopefully before it rolls over)
  //digitalWriteFast(9, HIGH); // oscilloscope trigger


  dma1.clearComplete();
  dma2.clearComplete();
  dma3.clearComplete();
  uint32_t bufsize = stripLen*24;
  dma1.transferCount(bufsize);
  dma2.transferCount(bufsize);
  dma3.transferCount(bufsize);
  dma2.sourceBuffer((uint8_t *)frameBuffer, bufsize);

  // clear any pending event flags
  FTM2_SC = 0x80;
  FTM2_C0SC = 0xA9;       // clear any previous pending DMA requests
  FTM2_C1SC = 0xA9;
  // clear any prior pending DMA requests
  dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_OV);
  dma2.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_CH0);
  dma3.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_CH1);
  //GPIOD_PTOR = 0xFF;
  //GPIOD_PTOR = 0xFF;
  dma1.enable();
  dma2.enable();          // enable all 3 DMA channels
  dma3.enable();
  FTM2_SC = 0x188;
  //digitalWriteFast(9, LOW);
#endif
  //Serial1.print("3");
  interrupts();
  //Serial1.print("4");
}

DMAMEM int displayMemory[maxLedsPerStrip*6];
MonopodWS2811 monopodws;

#endif

class BladeBase {
public:
  // Returns number of LEDs in this blade.
  virtual int num_leds() const = 0;
  // Returns true if the blade is supposed to be on.
  // false while "turning off".
  virtual bool is_on() const = 0;
  // Set led 'led' to color 'c'.
  virtual void set(int led, Color c) = 0;
  // Returns true when a clash occurs.
  // Returns true only once.
  virtual bool clash() = 0;
  // Called to let the blade know that it's ok to
  // disable power now. (Usually called after is_on()
  // has returned false for some period of time.)
  virtual void allow_disable() = 0;

  virtual void Activate() = 0;
};

class BladeStyle {
public:
  virtual void activate() {}
  virtual void deactivate() {}
  virtual void run(BladeBase* blade) = 0;
};

BladeStyle *current_style = NULL;

void SetStyle(class BladeStyle* style) {
  if (current_style) current_style->deactivate();
  current_style = style;
  current_style->activate();
}

class StyleCharging : public BladeStyle {
public:
  void activate() override {
    Serial.println("Charging Style");
  }
  void run(BladeBase *blade) override {
    int black_mix = 128 + 100 * sin(millis() / 500.0);
    float volts = battery_monitor.battery();
    Color colors[] = {
      Color(0,255,0),   // Green > 4.0
      Color(0,0,255),
      Color(255,128,0),
      Color(255,0,0),
      Color(255,0,0) 
    };
    float x = (4.0 - volts) * 2.0;
    int i = floor(x);
    i = clampi32(i, 0, NELEM(colors) - 2);
    // Blend colors over 0.1 volts.
    int blend = (x - i) * 10 * 255;
    blend = clampi32(blend, 0, 255);
    Color c = colors[i].mix(colors[i + 1], blend);
    c = c.mix(Color(), black_mix);
    int num_leds = blade->num_leds();

    float min_volts = 2.7;
    float max_volts = 4.2;
    float pos = (volts - min_volts) * num_leds / (max_volts - min_volts);
    int p = pos * 32;
    for (int i = 0; i < num_leds; i++) {
      blade->set(i, Color().mix(c, max(0, 256 - abs(p - i * 32))));
    }
  };
};

// No need to templetize this one, as there are no arguments.
StyleCharging style_charging;

class StyleFire : public BladeStyle {
public:
  StyleFire(Color c1, Color c2) : c1_(c1), c2_(c2) {}

  void activate() override {
    Serial.println("Fire Style");
    for (size_t i = 0; i < NELEM(heat_); i++) heat_[i] = 0;
  }
  void run(BladeBase* blade) override {
    uint32_t m = millis();
    int num_leds = blade->num_leds();
    if (m - last_update_ >= 10) {
      last_update_ = m;

      // Note heat_[0] is tip of blade
      if (blade->clash()) {
	heat_[num_leds - 1] += 3000;
      } else if (blade->is_on()) {
	heat_[num_leds - 1] += random(random(random(1000)));
      }
      for (int i = 0; i < num_leds; i++) {
	int x = (heat_[i] * 5  + heat_[i+1] * 7 + heat_[i+2] * 4) >> 4;
	heat_[i] = max(x - random(0, 1), 0);
      }
    }
    bool zero = true;
    for (int i = 0; i < num_leds; i++) {
      int h = heat_[num_leds - 1 - i];
      Color c;
      if (h < 256) {
	c = Color().mix(c1_, h);
      } else if (h < 512) {
	c = c1_.mix(c2_, h - 256);
      } else if (h < 768) {
	c = c2_.mix(Color(255,255,255), h - 512);
      } else {
	c = Color(255,255,255);
      }
      if (h) zero = false;
      blade->set(i, c);
    }
    if (zero) blade->allow_disable();
  }

private:
  static uint32_t last_update_;
  static unsigned short heat_[maxLedsPerStrip + 2];
  Color c1_, c2_;
};

uint32_t StyleFire::last_update_ = 0;
unsigned short StyleFire::heat_[maxLedsPerStrip + 2];

template<int r1, int g1, int b1, int r2, int g2, int b2>
class StyleFire *StyleFirePtr() {
  static StyleFire style(Color(r1, g1, b1), Color(r2, g2, b2));
  return &style;
}

class StyleNormal : public BladeStyle {
public:
  StyleNormal(Color color, Color clash_color, uint32_t out_millis, uint32_t in_millis)
    : color_(color),
      clash_color_(clash_color),
      out_millis_(out_millis),
      in_millis_(in_millis) {}

  void activate() override {
    Serial.println("Normal Style");
  }
  void run(BladeBase* blade) override {
    int num_leds = blade->num_leds();
    uint32_t m = millis();
    if (blade->clash()) clash_millis_ = m;
    Color c = color_;
    // Clash effect
#if 1
    if (m - clash_millis_ < 40) {
      c = clash_color_;
    }
#else
    int clash_t = m - clash_millis_;
    int clash_mix = min(clash_t * 50, 512 - clash_t * 10);
    c = c.mix(clash_color_, clampi32(clash_mix, 0, 255));
#endif

    if (on_ != blade->is_on()) {
      event_millis_ = m;
      on_ = blade->is_on();
    }
    int thres = num_leds * 256 * (m - event_millis_) / (on_ ? out_millis_ : in_millis_);
    if (!blade->is_on()) thres = num_leds * 256 - thres;
    for (int i = 0; i < num_leds; i++) {
      int black_mix = clampi32(thres - i * 256, 0, 255);
      blade->set(i, Color().mix(c, black_mix));
    }
    if (!blade->is_on() && thres > 256 * (num_leds*2)) {
      Serial.println("Allow off.");
      blade->allow_disable();
    }
    if (m - event_millis_ > 100000) event_millis_ += 1000;
    if (m - clash_millis_ > 100000) clash_millis_ += 1000;
  }
private:
  static bool on_;
  static uint32_t event_millis_;
  static uint32_t clash_millis_;
  Color color_;
  Color clash_color_;
  uint32_t out_millis_;
  uint32_t in_millis_;
};

bool StyleNormal::on_ = false;
uint32_t StyleNormal::event_millis_ = 0;
uint32_t StyleNormal::clash_millis_ = 0;

// Arguments: color, clash color, turn-on/off time
template<int r1, int g1, int b1, int r2, int g2, int b2, int out_millis, int in_millis>
class StyleNormal *StyleNormalPtr() {
  static StyleNormal style(Color(r1, g1, b1), Color(r2, g2, b2), out_millis, in_millis);
  return &style;
}

class StyleRainbow : public BladeStyle {
public:
  StyleRainbow(uint32_t out_millis, uint32_t in_millis)
    : out_millis_(out_millis),
      in_millis_(in_millis) {}

  void activate() override {
    Serial.println("Normal Style");
  }
  void run(BladeBase* blade) override {
    int num_leds = blade->num_leds();
    uint32_t m = millis();
    if (blade->clash()) clash_millis_ = m;

    if (on_ != blade->is_on()) {
      event_millis_ = m;
      on_ = blade->is_on();
    }
    int clash_t = m - clash_millis_;
    int clash_mix = min(clash_t * 50, 512 - clash_t * 10);
    int thres = num_leds * 256 * (m - event_millis_) / (on_ ? out_millis_ : in_millis_);
    if (!blade->is_on()) thres = num_leds * 256 - thres;
    for (int i = 0; i < num_leds; i++) {
      int black_mix = clampi32(thres - i * 256, 0, 255);
      Color c(max(0, (sin_table[((m * 3 + i * 50)) & 0x3ff] >> 7)),
	      max(0, (sin_table[((m * 3 + i * 50 + 1024 / 3)) & 0x3ff] >> 7)),
	      max(0, (sin_table[((m * 3 + i * 50 + 1024 * 2 / 3)) & 0x3ff] >> 7)));
      c = c.mix(Color(255,255,255), clampi32(clash_mix, 0, 255));
      // Clash effect
      blade->set(i, Color().mix(c, black_mix));
    }
    if (!blade->is_on() && thres > 256 * (num_leds*2)) {
      Serial.println("Allow off.");
      blade->allow_disable();
    }
    if (m - event_millis_ > 100000) event_millis_ += 1000;
    if (m - clash_millis_ > 100000) clash_millis_ += 1000;
  }
private:
  static bool on_;
  static uint32_t event_millis_;
  static uint32_t clash_millis_;
  uint32_t out_millis_;
  uint32_t in_millis_;
};

bool StyleRainbow::on_ = false;
uint32_t StyleRainbow::event_millis_ = 0;
uint32_t StyleRainbow::clash_millis_ = 0;

// Arguments: color, clash color, turn-on/off time
template<int out_millis, int in_millis>
class StyleRainbow *StyleRainbowPtr() {
  static StyleRainbow style(out_millis, in_millis);
  return &style;
}

class StyleStrobe : public BladeStyle {
public:
  StyleStrobe(Color color,
	      Color clash_color,
	      int frequency,
	      uint32_t out_millis,
	      uint32_t in_millis)
    : color_(color),
      clash_color_(clash_color),
      strobe_millis_(1000/frequency),
      out_millis_(out_millis),
      in_millis_(in_millis) {}

  void activate() override {
    Serial.println("Strobe Style");
  }
  void run(BladeBase* blade) override {
    int num_leds = blade->num_leds();
    uint32_t m = millis();
    if (blade->clash()) clash_millis_ = m;
    Color c = Color();
    if (m == last_strobe_ || m - last_strobe_ > strobe_millis_) {
      c = color_;
      last_strobe_ = m;
    }
    // Clash effect
    if (m - clash_millis_ < 40) {
      c = clash_color_;
    }

    if (on_ != blade->is_on()) {
      event_millis_ = m;
      on_ = blade->is_on();
    }
    int thres = num_leds * 256 * (m - event_millis_) / (on_ ? out_millis_ : in_millis_);
    if (!blade->is_on()) thres = num_leds * 256 - thres;
    for (int i = 0; i < num_leds; i++) {
      int black_mix = clampi32(thres - i * 256, 0, 255);
      blade->set(i, Color().mix(c, black_mix));
    }
    if (!blade->is_on() && thres > 256 * (num_leds*2)) {
      Serial.println("Allow off.");
      blade->allow_disable();
    }
    if (m - event_millis_ > 100000) event_millis_ += 1000;
    if (m - clash_millis_ > 100000) clash_millis_ += 1000;
  }
private:
  static bool on_;
  static uint32_t event_millis_;
  static uint32_t clash_millis_;
  static uint32_t last_strobe_;
  Color color_;
  Color clash_color_;
  uint32_t strobe_millis_;
  uint32_t out_millis_;
  uint32_t in_millis_;
};

bool StyleStrobe::on_ = false;
uint32_t StyleStrobe::last_strobe_ = 0;
uint32_t StyleStrobe::event_millis_ = 0;
uint32_t StyleStrobe::clash_millis_ = 0;

// Arguments: color, clash color, turn-on/off time
template<int r1, int g1, int b1, int r2, int g2, int b2, int frequency, int out_millis, int in_millis>
class StyleStrobe *StyleStrobePtr() {
  static StyleStrobe style(Color(r1, g1, b1), Color(r2, g2, b2), frequency, out_millis, in_millis);
  return &style;
}


#if 0
class StylePOV : public BladeStyle {
public:
  StylePOV(Color* data, int width, int height)
    : data_(data), width_(width), height_(height) {
  }
  void activate() override {
    Serial.println("POV Style");
  }
  void run(BladeBase* blade) override {
    int col = ANGLE_TODO;
    int num_leds_ = blade->num_leds();
    int led = 0;
    while (led < num_leds - height_) blade->set(led++, Color());
    Color* data_ + height_ * col;
    for (int i = 0; i < min(height, num_leds); i++) {
      blade->set(led++, *(col++));
    }
  }
private:
  Color* data_;
  int width_;
  int height_;
};
#endif

#ifdef ENABLE_WS2811
// WS2811-type blade implementation.
// Note that this class does nothing when first constructed. It only starts
// interacting with pins and timers after Activate() is called.
class WS2811_Blade : public SaberBase, CommandParser, Looper, public BladeBase {
public:
  WS2811_Blade(int num_leds, uint8_t config) :
    SaberBase(NOLINK),
    CommandParser(NOLINK),
    Looper(NOLINK),
    num_leds_(num_leds),
    config_(config) {
  }

  void Power(bool on) {
    pinMode(bladePowerPin1, OUTPUT);
    pinMode(bladePowerPin2, OUTPUT);
    pinMode(bladePowerPin3, OUTPUT);
    digitalWrite(bladePowerPin1, on?HIGH:LOW);
    digitalWrite(bladePowerPin2, on?HIGH:LOW);
    digitalWrite(bladePowerPin3, on?HIGH:LOW);
//    pinMode(bladePin, on ? OUTPUT : INPUT);
    powered_ = on;
  }

  // No need for a "deactivate", the blade stays active until
  // you take it out, which also cuts the power.
  void Activate() override {
    Serial.print("WS2811 Blade with ");
    Serial.print(num_leds_);
    Serial.println(" leds");
    Power(true);
    delay(10);
    monopodws.begin(num_leds_, displayMemory, config_);
    monopodws.show();  // Make it black
    monopodws.show();  // Make it black
    monopodws.show();  // Make it black
    while (monopodws.busy());
    CommandParser::Link();
    Looper::Link();
    SaberBase::Link();
  }

  // BladeBase implementation
  int num_leds() const override {
    return num_leds_;
  }
  bool is_on() const override {
    return on_;
  }
  void set(int led, Color c) override {
    monopodws.setPixel(led, c);
  }
  bool clash() override {
    bool ret = clash_;
    clash_ = false;
    return ret;
  }
  void allow_disable() override {
    if (!on_) {
      Power(false);
    }
  }

  // SaberBase implementation.
  void IsOn(bool* on) override {
    if (on_) *on = true;
  }
  void On() override {
    Power(true);
    delay(10);
    on_ = true;
  }
  void Off() override {
    on_ = false;
  }

  void Clash() override { clash_=true; }
  void Lockup() override {  }

  void Top() override {
    if (!millis_sum_) return;
    Serial.print("blade fps: ");
    Serial.println(updates_ * 1000.0 / millis_sum_);
  }

  bool Parse(const char* cmd, const char* arg) override {
    if (!strcmp(cmd, "blade")) {
      if (!strcmp(arg, "on")) {
        On();
        return true;
      }
      if (!strcmp(arg, "off")) {
        Off();
        return true;
      }
    }
    return false;
  }

protected:
  void Loop() override {
    if (!powered_) {
      last_millis_ = 0;
      return;
    }
    if (monopodws.busy()) return;
    monopodws.show();
    int m = millis();
    if (last_millis_) {
      millis_sum_ += m - last_millis_;
      updates_ ++;
      if (updates_ > 1000) {
        updates_ /= 2;
        millis_sum_ /= 2;
      }
    }
    last_millis_ = m;
    current_style->run(this);
  }
  
private:
  int num_leds_;
  uint8_t config_;
  static bool on_;
  static bool powered_;
  static bool clash_;
  static int updates_;
  static int millis_sum_;
  static uint32_t last_millis_;
};

bool WS2811_Blade::on_ = false;
bool WS2811_Blade::powered_ = false;
bool WS2811_Blade::clash_ = false;
int WS2811_Blade::updates_ = 0;
int WS2811_Blade::millis_sum_ = 0;
uint32_t WS2811_Blade::last_millis_ = 0;

template<int LEDS, int CONFIG>
class WS2811_Blade *WS2811BladePtr() {
  static_assert(LEDS <= maxLedsPerStrip, "update maxLedsPerStrip");
  static WS2811_Blade blade(LEDS, CONFIG);
  return &blade;
}
#endif

class LEDInterface {
public:
  virtual int PWM(Color c) = 0;
};

template<class LED>
class DriveLogic : public LEDInterface {
public:
  float PWMMultiplier() {
    float V = battery_monitor.battery();
    float dv = LED::MaxVolts - LED::P2Volts;
    float di = LED::MaxAmps - LED::P2Amps;
    float delta = dv / di;
    float amps = (V - LED::MaxVolts + LED::MaxAmps * delta) / (delta + LED::R);
    if (amps <= LED::MaxAmps) {
      return 1.0f;
    }
    return LED::MaxAmps / amps;
  }
  int PWM(Color c) override {
    return c.select(Color(LED::Red, LED::Green, LED::Blue)) * PWMMultiplier();
  }
};

template<class LED>
class LEDInterface* LEDPtr() {
  static DriveLogic<LED> led;
  return &led;
}

// Simple blade, LED string or LED star with optional flash on clash.
// Note that this class does nothing when first constructed. It only starts
// interacting with pins and timers after Activate() is called.
class Simple_Blade : public SaberBase, CommandParser, Looper, public BladeBase {
public:
  Simple_Blade(LEDInterface* c1,
	       LEDInterface* c2,
	       LEDInterface* c3,
	       LEDInterface* c4) :
    SaberBase(NOLINK),
    CommandParser(NOLINK),
    Looper(NOLINK),
    c1_(c1), 
    c2_(c2),
    c3_(c3),
    c4_(c4) {
  }

  void Activate() override {
    Serial.println("Simple Blade");
    analogWriteResolution(8);
    analogWriteFrequency(bladePowerPin1, 1000);
    analogWriteFrequency(bladePowerPin2, 1000);
    analogWriteFrequency(bladePowerPin3, 1000);
    analogWriteFrequency(bladePin, 1000);
    analogWrite(bladePowerPin1, 0);  // make it black
    analogWrite(bladePowerPin2, 0);  // make it black
    analogWrite(bladePowerPin3, 0);  // make it black
    analogWrite(bladePin, 0);        // make it black
    CommandParser::Link();
    Looper::Link();
    SaberBase::Link();
  }

  // BladeBase implementation
  int num_leds() const override {
    return 1;
  }
  bool is_on() const override {
    return on_;
  }
  void set(int led, Color c) override {
    analogWrite(bladePowerPin1, c1_->PWM(c));
    analogWrite(bladePowerPin2, c2_->PWM(c));
    analogWrite(bladePowerPin3, c3_->PWM(c));
    analogWrite(bladePin, c4_->PWM(c));
  }

  bool clash() override {
    bool ret = clash_;
    clash_ = false;
    return ret;
  }
  void allow_disable() override {
    power_ = false;
  }
  
  // SaberBase implementation
  void IsOn(bool *on) override {
    if (on_) *on = true;
  }
  void On() override { power_ = on_ = true; }
  void Off() override { on_ = false; }
  void Clash() override {
    clash_ = true;
  }
  void Lockup() override {  }

  bool Parse(const char* cmd, const char* arg) override {
    if (!strcmp(cmd, "blade")) {
      if (!strcmp(arg, "on")) {
        On();
        return true;
      }
      if (!strcmp(arg, "off")) {
        Off();
        return true;
      }
    }
    return false;
  }

protected:
  void Loop() override {
    if (!power_) return;
    current_style->run(this);
  }
  
private:
  LEDInterface *c1_;
  LEDInterface *c2_;
  LEDInterface *c3_;
  LEDInterface *c4_;
  static bool on_;
  static bool power_;
  static bool clash_;
};

bool Simple_Blade::on_ = false;
bool Simple_Blade::power_ = false;
bool Simple_Blade::clash_ = true;

template<class LED1, class LED2, class LED3, class LED4>
class Simple_Blade *SimpleBladePtr() {
  static Simple_Blade blade(LEDPtr<LED1>(), LEDPtr<LED2>(), LEDPtr<LED3>(), LEDPtr<LED4>());
  return &blade;
}


#ifdef V2
// String blade, Segment LED string. All segments assumed to be the same color.
// Note that this class does nothing when first constructed. It only starts
// interacting with pins and timers after Activate() is called.
#define STRING_SEGMENTS 6
class String_Blade : public SaberBase, CommandParser, Looper, public BladeBase {
public:
  String_Blade(Color c) :
    SaberBase(NOLINK),
    CommandParser(NOLINK),
    Looper(NOLINK),
    c_(c) {
  }

  void Activate() override {
    Serial.println("String Blade");
    analogWriteResolution(8);
    for (int i = 0; i < STRING_SEGMENTS; i++) {
      analogWriteFrequency(pin_[i], 1000);
      analogWrite(pin_[i], 0);  // make it black
    }
    CommandParser::Link();
    Looper::Link();
    SaberBase::Link();
  }

  // BladeBase implementation
  int num_leds() const override {
    return STRING_SEGMENTS;
  }
  bool is_on() const override {
    return on_;
  }
  void set(int led, Color c) override {
    analogWrite(pin_[led], c.select(c_));
  }

  bool clash() override {
    bool ret = clash_;
    clash_ = false;
    return ret;
  }
  void allow_disable() override {
    power_ = false;
  }
  
  // SaberBase implementation
  void IsOn(bool *on) override {
    if (on_) *on = true;
  }
  void On() override { power_ = on_ = true; }
  void Off() override { on_ = false; }
  void Clash() override {
    clash_ = true;
  }
  void Lockup() override {  }

  bool Parse(const char* cmd, const char* arg) override {
    if (!strcmp(cmd, "blade")) {
      if (!strcmp(arg, "on")) {
        On();
        return true;
      }
      if (!strcmp(arg, "off")) {
        Off();
        return true;
      }
    }
    return false;
  }

protected:
  void Loop() override {
    if (!power_) return;
    current_style->run(this);
  }
  
private:
  Color c_;
  static int pin_[STRING_SEGMENTS];
  static bool on_;
  static bool power_;
  static bool clash_;
};

int String_Blade::pin_[STRING_SEGMENTS] = {
    bladePowerPin1,
    bladePowerPin2,
    bladePowerPin3,
    bladePowerPin4,
    bladePowerPin5,
    bladePowerPin6,
};
bool String_Blade::on_ = false;
bool String_Blade::power_ = false;
bool String_Blade::clash_ = true;
#endif

template<int r1, int g1, int b1>
class String_Blade *StringBladePtr() {
  static String_Blade blade(Color(r1, g1, b1));
  return &blade;
}

#define RED       255,   0,   0
#define GREEN       0, 255,   0
#define BLUE        0,   0, 255
#define YELLOW    255, 255,   0
#define CYAN        0, 255, 255
#define MAGENTA   255,   0, 255
#define WHITE     255, 255, 255
#define BLACK       0,   0,   0

// CONFIGURABLE
struct CreeXPE2White {
  static constexpr float MaxAmps = 1.0;
  static constexpr float MaxVolts = 3.15;
  static constexpr float P2Amps = 0.7;
  static constexpr float P2Volts = 3.05;
  static constexpr float R = 0.55;
  static const int Red = 255;
  static const int Green = 255;
  static const int Blue = 255;
};

struct CreeXPE2Blue {
  static constexpr float MaxAmps = 1.0;
  static constexpr float MaxVolts = 3.4;
  static constexpr float P2Amps = 0.35;
  static constexpr float P2Volts = 3.1;
  static constexpr float R = 0.24;
  static const int Red = 0;
  static const int Green = 0;
  static const int Blue = 255;
};

struct CreeXPE2Green {
  static constexpr float MaxAmps = 1.0;
  static constexpr float MaxVolts = 3.7;
  static constexpr float P2Amps = 0.35;
  static constexpr float P2Volts = 3.2;
  static constexpr float R = 0.0;
  static const int Red = 0;
  static const int Green = 255;
  static const int Blue = 0;
};

struct CreeXPE2PCAmber {
  static constexpr float MaxAmps = 1.0;
  static constexpr float MaxVolts = 3.28;
  static constexpr float P2Amps = 0.35;
  static constexpr float P2Volts = 3.05;
  static constexpr float R = 0.0;
  // TODO(hubbe): Find correct values for PC Amber
  static const int Red = 255;
  static const int Green = 128;
  static const int Blue = 0;
};

struct CreeXPE2Red {
  static constexpr float MaxAmps = 1.0;
  static constexpr float MaxVolts = 2.65;
  static constexpr float P2Amps = 0.35;
  static constexpr float P2Volts = 2.2;
  static constexpr float R = 0.0;
  static const int Red = 255;
  static const int Green = 0;
  static const int Blue = 0;
};

struct CreeXPE2RedOrange {
  static constexpr float MaxAmps = 1.0;
  static constexpr float MaxVolts = 2.65;
  static constexpr float P2Amps = 0.35;
  static constexpr float P2Volts = 2.2;
  static constexpr float R = 0.0;
  // TODO(hubbe): Find correct values for red-orange
  static const int Red = 255;
  static const int Green = 196;
  static const int Blue = 0;
};

struct CreeXPE2Amber {
  static constexpr float MaxAmps = 1.0;
  static constexpr float MaxVolts = 2.65;
  static constexpr float P2Amps = 0.35;
  static constexpr float P2Volts = 2.2;
  static constexpr float R = 0.0;
  // TODO(hubbe): Find correct values for Amber
  static const int Red = 255;
  static const int Green = 100;
  static const int Blue = 0;
};

struct Blue3mmLED {
  // TODO(hubbe): Measure these.
  static constexpr float MaxAmps = 0.03;
  static constexpr float MaxVolts = 3.8;
  static constexpr float P2Amps = 0.0;
  static constexpr float P2Volts = 2.0;
  static constexpr float R = 0.0;
  static const int Red = 255;
  static const int Green = 0;
  static const int Blue = 0;
};

// For when there is no LED hooked up to a channel.
struct NoLED {
  static constexpr float MaxAmps = 1.0;
  static constexpr float MaxVolts = 1000.0;
  static constexpr float P2Amps = 0.0;
  static constexpr float P2Volts = 0.0;
  static constexpr float R = 0.0;
  static const int Red = 0;
  static const int Green = 0;
  static const int Blue = 0;
};


#define CONFIGARRAY(X) X, NELEM(X)

struct Preset {
  // Sound font.
  const char* font;

  // Sound track
  const char* track;

  // Blade config.
  BladeStyle* style;
};

// CONFIGURABLE
// Each preset line consists of:
// { "font directory", "sound track directory", Style },
// Where Style is one of:
//   StyleNormalPtr<BaseColor, FlashColor, out millis, in millis>()
//   StyleFirePtr<LowHeatColor, HighHeatColor>()
//   StyleRainBowPtr<out millis, in millis>()
//   StyleStrobePtr<BaseColor, FlashColor, out millis, in millis>()
//   &style_charging
// All colors can be specied as three numbers or using one the handy macros above.
// If you wish to have different presets for different blades, copy this array and
// name it something other than "preset", then use the new name inside the blade
// configuration array below. See "simple_presets" and "charging_presets" below for
// examples.
Preset presets[] = {
  { "font01", "tracks/title.wav", StyleNormalPtr<CYAN, WHITE, 300, 800>() },
  { "font01", "tracks/cantina.wav", StyleNormalPtr<BLUE, RED, 300, 800>() },
  { "caliban", "tracks/duel.wav", StyleFirePtr<RED, YELLOW>() },
  { "igniter/font2", "tracks/vader.wav", StyleNormalPtr<RED, WHITE, 300, 800>() },
  { "font02", "tracks/title.wav", StyleFirePtr<BLUE, CYAN>() },
  { "igniter/font4", "tracks/duel.wav", StyleNormalPtr<GREEN, WHITE, 300, 800>() },
  { "font01", "tracks/duel.wav", StyleNormalPtr<WHITE, RED, 300, 800>() },
  { "font01", "tracks/walls.wav", StyleNormalPtr<YELLOW, BLUE, 300, 800>() },
  { "font01", "tracks/title.wav", StyleNormalPtr<MAGENTA, WHITE, 300, 800>() },
  { "font02", "tracks/cantina.wav", StyleRainbowPtr<300, 800>() },
  { "font02", "tracks/cantina.wav", StyleStrobePtr<WHITE, RED, 15, 300, 800>() },
  { "charging", "tracks/duel.wav", &style_charging },
};

Preset simple_presets[] = {
  { "font01", "tracks/title.wav", StyleNormalPtr<BLUE, WHITE, 100, 200>() },
  { "font02", "tracks/duel.wav", StyleNormalPtr<BLUE, WHITE, 100, 200>() },
  { "font02", "tracks/cantina.wav", StyleStrobePtr<BLUE, WHITE, 15, 100, 200>() },
};

Preset charging_presets[] = {
  { "charging", "", &style_charging },
  { "font01", "tracks/title.wav", StyleNormalPtr<BLUE, BLUE, 100, 200>() },
  { "font02", "tracks/duel.wav", StyleNormalPtr<BLUE, BLUE, 100, 200>() },
};

Preset testing_presets[] = {
  { "font02", "tracks/cantina.wav", StyleRainbowPtr<300, 800>() },
};

struct BladeConfig {
  // Blade identifier resistor.
  int ohm;

  // Blade driver.
  BladeBase* blade;

  // Blade presets
  Preset* presets;
  size_t num_presets;
};

// CONFIGURABLE
// Each line of configuration should be:
//     { blade id resistor ohms, blade, CONFIGARRAY(array of presets) },
// Where "blade", can be one of the following:
//     WS2811BladePtr<number of leds, WS2811 configuration flags>()
//     SimpleBladePTR<Color of channel 1, color of channel 2, color of channel 3, color of channel 4>()
// All colors can be specied as three numbers or using one the handy macros above.

BladeConfig blades[] = {
#ifdef ENABLE_WS2811
  // PL9823 blade, 97 LEDs
  {  69000, WS2811BladePtr<97, WS2811_580kHz>(), CONFIGARRAY(presets) },
  {   2600, WS2811BladePtr<97, WS2811_580kHz>(), CONFIGARRAY(presets) },
#endif

  // Simple blue string blade.
  {   5200, SimpleBladePtr<Blue3mmLED,Blue3mmLED,Blue3mmLED,NoLED>(), CONFIGARRAY(simple_presets) },

#ifdef ENABLE_WS2811
  // Charging adapter, single PL9823 LED.
  {  41000, WS2811BladePtr<1, WS2811_580kHz>(), CONFIGARRAY(charging_presets) },
  {  15000, WS2811BladePtr<1, WS2811_580kHz>(), CONFIGARRAY(charging_presets) },

//  {  69000, WS2811BladePtr<140, WS2811_800kHz>(), CONFIGARRAY(presets) },
  {   7800, WS2811BladePtr<144, WS2811_800kHz | WS2811_GRB>(), CONFIGARRAY(presets) },
#endif

  // Blue-Blue-White LED star
  { 20000, SimpleBladePtr<CreeXPE2White, CreeXPE2Blue, CreeXPE2Blue, NoLED>(), CONFIGARRAY(simple_presets) },

  // Testing configuration. 
  { 130000, SimpleBladePtr<CreeXPE2Red, CreeXPE2Green, CreeXPE2Blue, NoLED>(), CONFIGARRAY(testing_presets) },
};


#ifdef ENABLE_SERIALFLASH
// Support for uploading files in TAR format.
class Tar {
  public:
    // Header description:
    //
    // Fieldno  Offset  len     Description
    // 
    // 0        0       100     Filename
    // 1        100     8       Mode (octal)
    // 2        108     8       uid (octal)
    // 3        116     8       gid (octal)
    // 4        124     12      size (octal)
    // 5        136     12      mtime (octal)
    // 6        148     8       chksum (octal)
    // 7        156     1       linkflag
    // 8        157     100     linkname
    // 9        257     8       magic
    // 10       265     32      (USTAR) uname
    // 11       297     32      (USTAR) gname
    // 12       329     8       devmajor (octal)
    // 13       337     8       devminor (octal)
    // 14       345     167     (USTAR) Long path
    //
    // magic can be any of:
    //   "ustar\0""00"  POSIX ustar (Version 0?).
    //   "ustar  \0"    GNU tar (POSIX draft)

  void begin() {
    file_length_ = 0;
    bytes_ = 0;
  }
  bool write(char* data, size_t bytes) {
    while (bytes) {
      size_t to_copy = min(sizeof(block_) - bytes_, bytes);
      memcpy(block_ + bytes_, data, to_copy);
      bytes -= to_copy;
      data += to_copy;
      bytes_ += to_copy;
//      Serial.print("Have: ");
//      Serial.println(bytes_);

      if (bytes_ == sizeof(block_)) {
        if (file_length_) {
          size_t to_write = min(sizeof(block_), file_length_);
          file_.write(block_, to_write);
          file_length_ -= to_write;
	  if (!file_length_) file_.close();
        } else {
          if (memcmp("ustar", block_ + 257, 5)) {
            Serial.println("NOT USTAR!");
          } else {
            // Start new file here
            file_length_ = strtol(block_ + 124, NULL, 8);
            Serial.print("Receiving ");
            Serial.print(block_);
            Serial.print(" length = ");
            Serial.println(file_length_);
#ifdef TAR_UPLOADS_TO_SDCARD
	    file_ = SD.open(block_, FILE_WRITE);
#else
            if (!SerialFlashChip::create(block_, file_length_)) {
              Serial.println("Create file failed.");
              return false;
            }
            file_ = SerialFlashChip::open(block_);
#endif
          }
        }
        bytes_ = 0;
      }
    }
    return true;
  }
  private:
#ifdef TAR_UPLOADS_TO_SDCARD
    File file_;
#else
    SerialFlashFile file_;
#endif
    size_t file_length_;
    size_t bytes_;

    // TODO: Can this be shared with some other task?
    char block_[512];
};

#endif

class DebouncedButton : StateMachine {
public:
  void Update() {
    STATE_MACHINE_BEGIN();
    while (true) {
      while (!Read()) YIELD();
      pushed_ = true;
      do {
	if (Read()) last_on_ = millis();
	YIELD();
      } while (millis() - last_on_ < timeout());
      pushed_ = false;
    }
    STATE_MACHINE_END();
  }
  bool DebouncedRead() {
    Update();
    return pushed_;
  }

protected:
  virtual uint32_t timeout() { return 10; }
  virtual bool Read() = 0;

private:
  uint32_t last_on_;
  bool pushed_ = false;
};

// Simple button handler. Keeps track of clicks and lengths of pushes.
class ButtonBase : public Looper,
		   public CommandParser,
		   public DebouncedButton {
public:
  ButtonBase(const char* name)
    : Looper(),
      CommandParser(),
      name_(name),
      pushed_(false),
      clicked_(false),
      long_clicked_(false),
      push_millis_(0) {
  }
  int pushed_millis() {
    if (pushed_) return millis() - push_millis_;
    return 0;
  }
  bool clicked() {
    bool ret = clicked_;
    clicked_ = false;
    return ret;
  }

  bool long_clicked() {
    bool ret = long_clicked_;
    long_clicked_ = false;
    return ret;
  }

  void EatClick() {
    eat_click_ = true;
  }

protected:
  void Loop() {
    STATE_MACHINE_BEGIN();
    while (true) {
      while (!DebouncedRead()) YIELD();
      pushed_ = true;
      push_millis_ = millis();
      while (DebouncedRead()) YIELD();
      pushed_ = false;
      if (eat_click_) {
	eat_click_ = false;
      } else {
	if (millis() - push_millis_ < 500) {
	  clicked_ = true;
	} else {
	  long_clicked_ = true;
	}
      }
    }
    STATE_MACHINE_END();
  }
  bool Parse(const char* cmd, const char* arg) override {
    if (!strcmp(cmd, name_)) {
      clicked_ = true;
      return true;
    }
    return false;
  }

  int next_state_ = -1;
  uint32_t sleep_until_ = 0;
  const char* name_;
  bool pushed_;
  bool clicked_;
  bool long_clicked_;
  bool eat_click_ = false;
  int push_millis_;
};

class Button : public ButtonBase {
public:
  Button(int pin, const char* name) : ButtonBase(name), pin_(pin) {
    pinMode(pin, INPUT_PULLUP);
#ifdef ENABLE_SNOOZE
    snooze_config.pinMode(pin, INPUT_PULLUP, RISING);
#endif
  }
protected:
  bool Read() override {
    return digitalRead(pin_) == LOW;
  }
  uint8_t pin_;
};

// What follows is a copy of the touch.c code from the TensyDuino core library.
// That code originally implements the touchRead() function, I have modified it
// to become a class instead. That way reading the touch sensor can be
// initiated and polled without waiting around for it.

/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2013 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows 
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if defined(__MK20DX128__) || defined(__MK20DX256__)
// These settings give approx 0.02 pF sensitivity and 1200 pF range
// Lower current, higher number of scans, and higher prescaler
// increase sensitivity, but the trade-off is longer measurement
// time and decreased range.
#define CURRENT   2 // 0 to 15 - current to use, value is 2*(current+1)
#define NSCAN     9 // number of times to scan, 0 to 31, value is nscan+1
#define PRESCALE  2 // prescaler, 0 to 7 - value is 2^(prescaler+1)
static const uint8_t pin2tsi[] = {
//0    1    2    3    4    5    6    7    8    9
  9,  10, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255,  13,   0,   6,   8,   7,
255, 255,  14,  15, 255,  12, 255, 255, 255, 255,
255, 255,  11,   5
};

#elif defined(__MK66FX1M0__)
#define CURRENT   2
#define NSCAN     9
#define PRESCALE  2
static const uint8_t pin2tsi[] = {
//0    1    2    3    4    5    6    7    8    9
  9,  10, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255,  13,   0,   6,   8,   7,
255, 255,  14,  15, 255, 255, 255, 255, 255,  11,
 12, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

#elif defined(__MKL26Z64__)
#define NSCAN     9
#define PRESCALE  2
static const uint8_t pin2tsi[] = {
//0    1    2    3    4    5    6    7    8    9
  9,  10, 255,   2,   3, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255,  13,   0,   6,   8,   7,
255, 255,  14,  15, 255, 255, 255
};

#endif

// Note, there can currently only be one of these.
// If we need more, we need a time-sharing system.
class TouchButton : public ButtonBase {
public:
  TouchButton(int pin, int threshold, const char* name)
    : ButtonBase(name),
      pin_(pin),
      threshold_(threshold) {
    pinMode(pin, INPUT_PULLUP);
#ifdef ENABLE_SNOOZE
    snooze_config.pinMode(pin, TSI, threshold);
#endif
#if defined(__MK64FX512__)
    Serial.println("Touch sensor not supported!\n");
#endif
    if (pin >= NUM_DIGITAL_PINS) {
      Serial.println("touch pin out of range");
      return;
    }
    if (pin2tsi[pin_] == 255) {
      Serial.println("Not a touch-capable pin!");
    } 
  }
protected:

  void BeginRead() {
    // Copied from touch.c
    int32_t ch = pin2tsi[pin_];
    *portConfigRegister(pin_) = PORT_PCR_MUX(0);
    SIM_SCGC5 |= SIM_SCGC5_TSI;

#if defined(KINETISK) && !defined(HAS_KINETIS_TSI_LITE)
    TSI0_GENCS = 0;
    TSI0_PEN = (1 << ch);
    TSI0_SCANC = TSI_SCANC_REFCHRG(3) | TSI_SCANC_EXTCHRG(CURRENT);
    TSI0_GENCS = TSI_GENCS_NSCN(NSCAN) | TSI_GENCS_PS(PRESCALE) | TSI_GENCS_TSIEN | TSI_GENCS_SWTS;
#elif defined(KINETISL) || defined(HAS_KINETIS_TSI_LITE)
    TSI0_GENCS = TSI_GENCS_REFCHRG(4) | TSI_GENCS_EXTCHRG(3) | TSI_GENCS_PS(PRESCALE)
      | TSI_GENCS_NSCN(NSCAN) | TSI_GENCS_TSIEN | TSI_GENCS_EOSF;
    TSI0_DATA = TSI_DATA_TSICH(ch) | TSI_DATA_SWTS;
#endif
    begin_read_micros_ = micros();
  }

  void Setup() override {
    ButtonBase::Setup();
    BeginRead();
  }

  bool Read() override {
    return is_pushed_;
  }

  virtual uint32_t timeout() {
    return 50;
  }

  void Update(int value) {
    if (print_next_) {
      Serial.print("Touch ");
      Serial.print(name_);
      Serial.print(" = ");
      Serial.print(value);
      Serial.print(" (");
      Serial.print(min_);
      Serial.print(" - ");
      Serial.print(max_);
      Serial.println(")");
      
      print_next_ = false;
      min_ = 10000000;
      max_ = 0;
    } else {
      min_ = min(value, min_);
      max_ = max(value, max_);
    }
    is_pushed_ = value > threshold_;
  }

  // TODO(hubbe): Convert to state machine.
  void Loop() override {
    ButtonBase::Loop();
    if (monitor.ShouldPrint(Monitoring::MonitorTouch)) {
      print_next_ = true;
    }
    if (micros() - begin_read_micros_ <= 10) return;
    if (TSI0_GENCS & TSI_GENCS_SCNIP) return;
    delayMicroseconds(1);
#if defined(KINETISK) && !defined(HAS_KINETIS_TSI_LITE)
    int32_t ch = pin2tsi[pin_];
    Update(*((volatile uint16_t *)(&TSI0_CNTR1) + ch));
#elif defined(KINETISL) || defined(HAS_KINETIS_TSI_LITE)
    Update(TSI0_DATA & 0xFFFF);
#endif
    BeginRead();
  }

  bool print_next_ = false;
  int begin_read_micros_ = 0;
  uint8_t pin_;
  int threshold_;
  int min_ = 100000000;
  int max_ = 0;
  bool is_pushed_ = false;
};

// Menu system

// Configuration system:
// switch track
// + BLADE
// +-+ effect
// +-+ color
// +-+ sound font

// STATE_OFF:
// Power click  -> STATE_ON
// Power double click
// Power long click
// AUX click  -> STATE_ON
// AUX double click -> MUSIC_ON
// AUX long click -> SILENT_ON

// STATE_RUN:
// 

// The Saber class implements the basic states and actions
// for the saber.
class Saber : CommandParser, Looper {
public:
  Saber() : CommandParser(),
  // CONFIGURABLE, use "monitor touch" to see the range of
  // values from the touch sensor, then select a value that is
  // big enough to not trigger the touch sensor randomly.
            power_(powerButtonPin, 1700, "pow"),
            aux_(auxPin, "aux"),
            aux2_(aux2Pin, "aux2") {}

  bool IsOn() const {
    return on_;
  }

  void On() {
    if (on_) return;
    Serial.println("Ignition.");
    digitalWrite(amplifierPin, HIGH); // turn on the amplifier
    delay(10);             // allow time to wake up

    on_ = true;
    SaberBase::DoOn();
  }

  void Off() {
    on_ = false;
    SaberBase::DoOff();
  }

  unsigned long last_clash = 0;
  void Clash() {
    // TODO: Pick clash randomly and/or based on strength of clash.
    if (!on_) return;
    unsigned long t = millis();
    if (t - last_clash < 100) return;
    last_clash = t;
    SaberBase::DoClash();
  }

  bool chdir(const char* dir) {
    if (strlen(dir) > 1 && dir[strlen(dir)-1] == '/') {
      Serial.println("Directory must not end with slash.");
      return false;
    }
#ifdef ENABLE_AUDIO
    monophonic_font.Deactivate();
    polyphonic_font.Deactivate();

    // Stop all sound!
    // TODO: Move scanning to wav-playing interrupt level so we can
    // interleave things without worry about memory corruption.
    audio_splicer.Stop();
    for (size_t i = 0; i < NELEM(wav_players); i++) {
      wav_players[i].Stop();
    }
#endif

    strcpy(current_directory, dir);
    if (strlen(current_directory) && 
        current_directory[strlen(current_directory)-1] != '/') {
      strcat(current_directory, "/");
    }

#ifdef ENABLE_AUDIO
    Effect::ScanDirectory(dir);
    if (clsh.files_found()) {
      polyphonic_font.Activate();
      return true;
    }
    if (clash.files_found()) {
      monophonic_font.Activate();
      return true;
    }
    if (boot.files_found()) {
      monophonic_font.Activate();
      return false;
    }
#endif
    return false;
  }
  
  // Select preset (font/style)
  void SetPreset(Preset* preset) {
    current_preset_ = preset;
    SetStyle(preset->style);
    chdir(preset->font);
  }

  // Go to the next Preset.
  void next_preset() {
    digitalWrite(amplifierPin, HIGH); // turn on the amplifier
#ifdef ENABLE_AUDIO
    beeper.Beep(0.05, 2000.0);
#endif
    Preset* tmp = current_preset_ + 1;
    if (tmp == current_config_->presets + current_config_->num_presets) {
      tmp = current_config_->presets;
    }
    SetPreset(tmp);
    SaberBase::DoNewFont();
  }

  // Measure and return the blade identifier resistor.
  float id() {
    pinMode(bladeIdentifyPin, INPUT_PULLUP);
    int blade_id = analogRead(bladeIdentifyPin);
    float volts = blade_id * 3.3 / 1024.0;  // Volts at bladeIdentifyPin
    float amps = (3.3 - volts) / 33000;     // Pull-up is 33k
    float resistor = volts / amps;
    Serial.print("ID: ");
    Serial.print(blade_id);
    Serial.print(" volts ");
    Serial.print(volts);
    Serial.print(" resistance= ");
    Serial.println(resistor);
    return resistor;
  }

  // Called from setup to identify the blade and select the right
  // Blade driver, style and sound font.
  void FindBlade() {
    pinMode(bladeIdentifyPin, INPUT_PULLUP);
    delay(100);
    float resistor = id();
    size_t best_config = 0;
    float best_err = 1000000.0;
    for (size_t i = 0; i < sizeof(blades) / sizeof(blades)[0]; i++) {
      float err = fabs(resistor - blades[i].ohm);
      if (err < best_err) {
	best_config = i;
	best_err = err;
      }
    }
    Serial.print("blade= ");
    Serial.println(best_config);
    current_config_ = blades + best_config;
    current_config_->blade->Activate();
    SetPreset(current_config_->presets);
  }

  // Select next sound font (in alphabetic order)
  // Set sign to -1 to get the previous sound font instead.
  void next_directory(int sign = 1) {
#ifdef ENABLE_SD
    int tries = 0;
    int dirs = 0;
    do {
      dirs = 0;
      File dir = SD.open("/");
      File best, first;
      while (File f = dir.openNextFile()) {
	if (!f.isDirectory()) continue;
	dirs++;
	if (!first) {
	  first = f;
	} else {
	  if (cmpdir(f.name(), first.name())*sign < 0) first = f;
	}
	if (cmpdir(f.name(), current_directory)*sign <= 0) continue;
	if (best && cmpdir(f.name(), best.name())*sign > 0) continue;
	best = f;
      }
      if (best) {
	if (chdir(best.name())) {
	  SaberBase::DoNewFont();
	  return;
	}
      } else if (first) {
	if (chdir(first.name())) {
	  SaberBase::DoNewFont();
	  return;
	}
      }
    } while (++tries <= dirs);
#endif
  }

protected:
  int track_player_ = -1;

  void StartOrStopTrack() {
#ifdef ENABLE_AUDIO
    if (track_player_ >= 0) {
      wav_players[track_player_].Stop();
      track_player_ = -1;
    } else {
      digitalWrite(amplifierPin, HIGH); // turn on the amplifier
      for (int unit = NELEM(wav_players) - 1; unit >= 0; unit--) {
	if (!wav_players[unit].isPlaying()) {
	  track_player_ = unit;
	  wav_players[unit].Play(current_preset_->track);
	  return;
	}
      }
      Serial.println("No available WAV players.");
    }
#else
    Serial.println("Audio disabled.");
#endif
  }

  bool aux_on_ = true;
  bool lockup_ = false;
  uint32_t last_beep_;

  void Loop() override {
    if (battery_monitor.low()) {
      if (on_) {
	Off();
      } else if (millis() - last_beep_ > 1000) {
	if (current_style != &style_charging) {
	  Serial.println("Battery low beep");
#ifdef ENABLE_AUDIO
	  beeper.Beep(0.5, 440.0);
#endif
	}
      }
      last_beep_ = millis();
    }
    bool disable_lockup_ = true;
#ifdef ENABLE_AUDIO
    if (track_player_ >= 0 && !wav_players[track_player_].isPlaying()) {
      track_player_ = -1;
    }
#endif
    if (!on_) {
      if (power_.clicked()) {
	if (aux_.pushed_millis()) {
	  aux_.EatClick();
	  next_preset();
          Serial.println("Next preset");
	} else {
          Serial.println("On (power)");
	  On();
	  aux_on_ = false;
	}
      }
      if (power_.long_clicked()) {
	if (aux_.pushed_millis()) {
	  aux_.EatClick();
	  next_directory();
          Serial.println("next directory");
	} else {
	  StartOrStopTrack();
	}
      }
      if (aux_.clicked()) {
	aux_on_ = true;
	On();
        Serial.println("On (aux)");
      }
    } else {
      ButtonBase *a, *b;
      if (aux_on_) {
	a = &aux_;
	b = &power_;
      } else {
	b = &aux_;
	a = &power_;
      }
      if (a->clicked()) {
        Serial.println("Off");
        Off();
      }
      if (b->clicked()) {
        SaberBase::DoBlast();
        Serial.println("Blast");
      }
      if (a->long_clicked()) {
        Serial.println("Force");
        SaberBase::DoForce();
      }
      if (b->pushed_millis() > 500) {
	disable_lockup_ = false;
        if (!lockup_) {
          lockup_ = true;
          SaberBase::DoBeginLockup();
        }
      }
    }
    if (lockup_ && disable_lockup_) {
      lockup_ = false;
      SaberBase::DoEndLockup();
    }
  }

  bool Parse(const char *cmd, const char* arg) override {
    if (!strcmp(cmd, "id")) {
      id();
      return true;
    }
    if (!strcmp(cmd, "on")) {
      On();
      return true;
    }
    if (!strcmp(cmd, "off")) {
      Off();
      return true;
    }
    if (!strcmp(cmd, "next")) {
      if (!arg || (arg && !strcmp(arg, "preset"))) {
	next_preset();
	return true;
      }
      if (arg && !strcmp(arg, "font")) {
	next_directory();
	return true;
      }
    }
    if (!strcmp(cmd, "clash")) {
      Clash();
      return true;
    }
#ifdef ENABLE_AUDIO
    if (!strcmp(cmd, "beep")) {
      digitalWrite(amplifierPin, HIGH); // turn on the amplifier
      beeper.Beep(1.0, 3000.0);
      return true;
    }
    if (!strcmp(cmd, "play")) {
      if (!arg) {
	StartOrStopTrack();
	return true;
      }
      digitalWrite(amplifierPin, HIGH); // turn on the amplifier
      for (size_t unit = 0; unit < NELEM(wav_players); unit++) {
	if (!wav_players[unit].isPlaying()) {
	  wav_players[unit].Play(arg);
	  return true;
	}
      }
      Serial.println("No available WAV players.");
      return true;
    }
#endif
    if (!strcmp(cmd, "cd")) {
      chdir(arg);
      SaberBase::DoNewFont();
      return true;
    }
    if (!strcmp(cmd, "pwd")) {
      Serial.println(current_directory);
      return true;
    }
    if (!strcmp(cmd, "next") && arg && !strcmp(arg, "font")) {
      next_directory(1);
      return true;
    }
    if (!strcmp(cmd, "prev") && arg && !strcmp(arg, "font")) {
      next_directory(-1);
      return true;
    }
    return false;
  }
private:
  BladeConfig* current_config_ = NULL;
  Preset* current_preset_ = NULL;

  bool on_;
  TouchButton power_;
  Button aux_;
  Button aux2_;
};

Saber saber;

// Command-line parser. Easiest way to use it is to start the arduino
// serial monitor.
class Parser : Looper, StateMachine {
public:
  enum Mode {
    ModeCommand,
    ModeDecode
  };
  Parser() : Looper(), len_(0), mode_(ModeCommand) {}

  void Loop() override {
    STATE_MACHINE_BEGIN();
    while (true) {
      while (!Serial) YIELD();
      Serial.println("Welcome to TeensySaber, type 'help' for more info.");
      
      while (Serial) {
        while (!Serial.available()) YIELD();
        int c = Serial.read();
        if (c < 0) { len_ = 0; break; }
        if (c == '\n') { Parse(); len_ = 0; continue; }
        cmd_[len_] = c;
        cmd_[len_ + 1] = 0;
        if (len_ + 1 < (int)sizeof(cmd_)) len_++;
      }
    }
    STATE_MACHINE_END();
  }

  void Parse() {
#ifdef ENABLE_SERIALFLASH
    switch (mode_) {
      case ModeCommand:
        ParseCmd();
        break;
      case ModeDecode:
        UUDecode();
        break;
    }
#else
    ParseCmd();
#endif
  }

#ifdef ENABLE_SERIALFLASH
  int32_t DecodeChar(char c) {
    return (c - 32) & 0x3f;
  }

  void UUDecode() {
    if (cmd_[0] == '`') {
      Serial.println("Done");
      mode_ = ModeCommand;
      return;
    }
    int to_decode = DecodeChar(cmd_[0]);
    if (to_decode > 0 || to_decode < 80) {
      char *in = cmd_+ 1;
      char *out = cmd_;
      int decoded = 0;
      while (decoded < to_decode) {
        int32_t bits = (DecodeChar(in[0]) << 18) | (DecodeChar(in[1]) << 12) | (DecodeChar(in[2]) << 6) | DecodeChar(in[3]);
        out[0] = bits >> 16;
        out[1] = bits >> 8;
        out[2] = bits;
#if 0
        Serial.print(bits);
        Serial.print("<");
        Serial.print((int)out[0]);
        Serial.print(",");
        Serial.print((int)out[1]);
        Serial.print(",");
        Serial.print((int)out[2]);
        Serial.print(">");
        Serial.println("");
#endif
        in += 4;
        out += 3;
        decoded += 3;
      }
      if (tar_.write(cmd_, to_decode))
        return;
    }

    Serial.println("decode failed");
    mode_ = ModeCommand;
  }
#endif

  void ParseCmd() {
    if (len_ == 0 || len_ == (int)sizeof(cmd_)) return;
    char *cmd = cmd_;
    while (*cmd == ' ') cmd++;
    char *e = cmd;
    while (*e != ' ' && *e) e++;
    if (*e) {
      *e = 0;
      e++;  // e is now argument (if any)
    }
    if (!strcmp(cmd, "help")) {
      Serial.println("General commands:");
      Serial.println("  on, off, blade on, blade off, clash, pow, aux, aux2");
      // Serial.println("  red, green, blue, yellow, cyan, magenta, white");
#ifdef ENABLE_SERIALFLASH
      Serial.println("Serial Flash memory management:");
      Serial.println("   ls, rm <file>, format, play <file>, effects");
      Serial.println("To upload files: tar cf - files | uuencode x >/dev/ttyACM0");
#endif
      Serial.println("Debugging commands:");
      Serial.println("   monitor swings, monitor samples, top, version");
      return;
    }

    if (!strcmp(cmd, "end")) {
      // End command ignored.
      return;
    }
#ifdef ENABLE_SERIALFLASH
    if (!strcmp(cmd, "begin")) {
      Serial.println("Begin UUdecode tar file.\n");
      // Filename is ignored.
      mode_ = ModeDecode;
      tar_.begin();
      return;
    }
    if (!strcmp(cmd, "ls")) {
      SerialFlashChip::opendir();
      uint32_t size;
      while (SerialFlashChip::readdir(cmd_, sizeof(cmd_), size)) {
        Serial.print(cmd_);
        Serial.print(" ");
        Serial.println(size);
      }
      Serial.println("Done listing files.");
      return;
    }
    if (!strcmp(cmd, "rm")) {
      if (SerialFlashChip::remove(e)) {
        Serial.println("Removed.\n");
      } else {
        Serial.println("No such file.\n");
      }
      return;
    }
    if (!strcmp(cmd, "format")) {
      Serial.print("Erasing ... ");
      SerialFlashChip::eraseAll();
      while (!SerialFlashChip::ready());
      Serial.println("Done");
      return;
    }
#endif
#ifdef ENABLE_SD
    if (!strcmp(cmd, "dir")) {
      File dir = SD.open(e ? e : current_directory);
      while (File f = dir.openNextFile()) {
        Serial.print(f.name());
        Serial.print(" ");
        Serial.println(f.size());
        f.close();
      }
      Serial.println("Done listing files.");
      return;
    }
#endif
#if defined(ENABLE_SD) && defined(ENABLE_SERIALFLASH)
    if (!strcmp(cmd, "cache")) {
      File f = SD.open(e);
      if (!f) {
	Serial.println("File not found.");
	return;
      }
      int bytes = f.size();
      if (!SerialFlashChip::create(e, bytes)) {
	Serial.println("Not enough space on serial flash chip.");
	return;
      }
      SerialFlashFile o = SerialFlashChip::open(e);
      while (bytes) {
	char tmp[256];
	int b = f.read(tmp, min(bytes, (int)NELEM(tmp)));
	o.write(tmp, b);
	bytes -= b;
      }
      Serial.println("Cached!");
      return;
    }
#endif
    if (!strcmp(cmd, "effects")) {
      Effect::ShowAll();
      return;
    }
#if 0
    if (!strcmp(cmd, "df")) {
      Serial.print(SerialFlashChip::capacity());
      Serial.println(" bytes available.");
      return;
    }
#endif
#ifdef ENABLE_AUDIO
#if 0
    if (!strcmp(cmd, "ton")) {
      digitalWrite(amplifierPin, HIGH); // turn on the amplifier
      dac.SetStream(&saber_synth);
      saber_synth.on_ = true;
      return;
    }
    if (!strcmp(cmd, "tof")) {
      saber_synth.on_ = false;
      return;
    }
#endif
    // TODO: Move to DAC
    if (!strcmp(cmd, "dacbuf")) {
      for (size_t i = 0; i < NELEM(DAC::dac_dma_buffer); i++) {
	Serial.print(DAC::dac_dma_buffer[i]);
	if ((i & 0xf) == 0xf)
	  Serial.println("");
	else
	  Serial.print(" ");
      }
      Serial.println("");
      return;
    }
    if (!strcmp(cmd, "dumpwav")) {
      int16_t tmp[32];
      wav_players[0].Stop();
      wav_players[0].read(tmp, NELEM(tmp));
      wav_players[0].Play(e);
      for (int j = 0; j < 32; j++) {
	int k = wav_players[0].read(tmp, NELEM(tmp));
	for (int i = 0; i < k; i++) {
	  Serial.print(tmp[i]);
	  Serial.print(" ");
	}
	Serial.println("");
      }
      wav_players[0].Stop();
      return;
    }
#endif
    if (!strcmp(cmd, "top")) {
      // TODO: list cpu usage for various objects.
      SaberBase::DoTop();
      return;
    }
    if (!strcmp(cmd, "version")) {
      Serial.println(version);
      return;
    }
    if (CommandParser::DoParse(cmd, e)) {
      return;
    }
    Serial.println("Whut?");
  }
private:
#ifdef ENABLE_SERIALFLASH
  Tar tar_;
#endif
  int len_;
  Mode mode_;
  char cmd_[256];
};

Parser parser;

// Simple 3D vector.
class Vec3 {
public:
  Vec3(){}
  Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
  Vec3 operator-(const Vec3& o) const {
    return Vec3(x - o.x, y - o.y, z - o.z);
  }
  float len2() const { return x*x + y*y + z*z; }
  float x, y, z;
};

#ifdef ENABLE_MOTION

#ifdef V2
#define I2C_TIMEOUT_MILLIS 300

class I2CDevice {
public:
  explicit I2CDevice(uint8_t address) : address_(address) {
    Wire.begin();
//    Wire.setClock(400000);
  }
  void writeByte(uint8_t reg, uint8_t data) {
    Wire.beginTransmission(address_);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
  }
  int readByte(uint8_t reg) {
    Wire.beginTransmission(address_);
    Wire.write(reg);
    Wire.endTransmission(true);
    Wire.requestFrom(address_, (uint8_t) 1);
    if (Wire.available() < 1) {
      uint32_t start = millis();
      while (Wire.available() < 1) {
	if (millis() - start > I2C_TIMEOUT_MILLIS) return -1;
      }
    }
    return Wire.read();
  }
  int readBytes(uint8_t reg, uint8_t* data, int bytes) {
    Wire.beginTransmission(address_);
    Wire.write(reg);
    Wire.endTransmission(true);
    Wire.requestFrom(address_, (uint8_t) bytes);
    if (Wire.available() < bytes) {
      uint32_t start = millis();
      while (Wire.available() < bytes) {
	if (millis() - start > I2C_TIMEOUT_MILLIS) return -1;
      }
    }
    for (int i = 0; i < bytes; i++) {
      data[i] = Wire.read();
    }		
    return bytes;
  }
  void boink() {
    address_++;
    Serial.print("ADDR: ");
    Serial.println(address_);
  }
private:
  uint8_t address_;
};

class LSM6DS3H : public I2CDevice, Looper {
public:
  enum Registers {
    FUNC_CFG_ACCESS = 0x1,
    SENSOR_SYNC_TIME_FRAME = 0x4,
    FIFO_CONTROL1 = 0x6,
    FIFO_CONTROL2 = 0x7,
    FIFO_CONTROL3 = 0x8,
    FIFO_CONTROL4 = 0x9,
    FIFO_CONTROL5 = 0xA,
    ORIENT_CFG_G = 0xB,
    INT1_CTRL = 0xD,
    INT2_CTRL = 0xE,
    WHO_AM_I = 0xF,
    CTRL1_XL = 0x10,
    CTRL2_G = 0x11,
    CTRL3_C = 0x12,
    CTRL4_C = 0x13,
    CTRL5_C = 0x14,
    CTRL6_C = 0x15,
    CTRL7_G = 0x16,
    CTRL8_XL = 0x17,
    CTRL9_XL = 0x18,
    CTRL10_C = 0x19,
    MASTER_CONFIG = 0x1A,
    WAKE_UP_SRC = 0x1B,
    TAP_SRC = 0x1C,
    D6D_SRC = 0x1D,
    STATUS_REG = 0x1E,
    STATUS_SPIAux = 0x1E,
    OUT_TEMP_L = 0x20,
    OUT_TEMP_H = 0x21,
    OUTX_L_G = 0x22,
    OUTX_H_G = 0x23,
    OUTY_L_G = 0x24,
    OUTY_H_G = 0x25,
    OUTZ_L_G = 0x26,
    OUTZ_H_G = 0x27,
    OUTX_L_XL = 0x28,
    OUTX_H_XL = 0x29,
    OUTY_L_XL = 0x2A,
    OUTY_H_XL = 0x2B,
    OUTZ_L_XL = 0x2C,
    OUTZ_H_XL = 0x2D,
    SENSORHUB1_REG = 0x2E,
    SENSORHUB2_REG = 0x2F,
    SENSORHUB3_REG = 0x30,
    SENSORHUB4_REG = 0x31,
    SENSORHUB5_REG = 0x32,
    SENSORHUB6_REG = 0x33,
    SENSORHUB7_REG = 0x34,
    SENSORHUB8_REG = 0x35,
    SENSORHUB9_REG = 0x36,
    SENSORHUB10_REG = 0x37,
    SENSORHUB11_REG = 0x38,
    SENSORHUB12_REG = 0x39,
    FIFO_STATUS1 = 0x3A,
    FIFO_STATUS2 = 0x3B,
    FIFO_STATUS3 = 0x3C,
    FIFO_STATUS4 = 0x3D,
    FIFO_DATA_OUT_L = 0x3E,
    FIFO_DATA_OUT_H = 0x3F,
    TIMESTAMP0_REG = 0x40,
    TIMESTAMP1_REG = 0x40,
    TIMESTAMP2_REG = 0x41,
    STEP_TIMESTAMP_L = 0x49,
    STEP_TIMESTAMP_H = 0x4A,
    STEP_COUNTER_L = 0x4B,
    STEP_COUNTER_H = 0x4C,
    SENSORHUB13_REG = 0x4D,
    SENSORHUB14_REG = 0x4E,
    SENSORHUB15_REG = 0x4F,
    SENSORHUB16_REG = 0x50,
    SENSORHUB17_REG = 0x51,
    SENSORHUB18_REG = 0x52,
    FUNC_SRC = 0x53,
    TAP_CFG = 0x58,
    TAP_THS_6D = 0x59,
    INT_DUR2 = 0x5A,
    WAKE_UP_THS = 0x5B,
    WAKE_UP_DUR = 0x5C,
    FREE_FALL = 0x5D,
    MD1_CFG = 0x5E,
    MD2_CFG = 0x5F,
    OUT_MAG_RAW_X_L = 0x66,
    OUT_MAG_RAW_X_H = 0x67,
    OUT_MAG_RAW_Y_L = 0x68,
    OUT_MAG_RAW_Y_H = 0x69,
    OUT_MAG_RAW_Z_L = 0x6A,
    OUT_MAG_RAW_Z_H = 0x6B,
    CTRL_SPIAux = 0x70
  };

  LSM6DS3H() : I2CDevice(106) {
  }

  void InitChip() {
    delay(1000);
    Serial.println("SETUP");
    int wai = readByte(WHO_AM_I);
    Serial.println(wai);
    if (wai == -1) {
      boink();
      return;
    }

    writeByte(CTRL1_XL, 0x80);  // 1.66kHz accel
    writeByte(CTRL2_G, 0x80);   // 1.66kHz gyro
    writeByte(CTRL3_C, 0x44);   // ?
    writeByte(CTRL4_C, 0x00);
    writeByte(CTRL5_C, 0x00);
    writeByte(CTRL6_C, 0x00);
    writeByte(CTRL7_G, 0x00);
    writeByte(CTRL8_XL, 0x00);
    writeByte(CTRL9_XL, 0x38);  // accel xyz enable
    writeByte(CTRL10_C, 0x38);  // gyro xyz enable
    Serial.println(readByte(WHO_AM_I));
    Serial.println("SETUP DONE");

    // Power up??
  }

  void Setup() override {
    InitChip();
  }

  void Loop() override {
    union {
      uint8_t buffer[6];
      struct { int16_t x, y, z; } xyz;
    } dataBuffer;
    
    int status_reg = readByte(STATUS_REG);
    if (status_reg == -1) {
      // motion fail, reboot motion chip.
      writeByte(CTRL3_C, 1);
      delay(20);
//      boink();
      InitChip();
      return;
    }
    if (status_reg & 0x1) {
      // Temp data available
      int16_t temp_data;
      if (readBytes(OUT_TEMP_L, (uint8_t*)&temp_data, 2) == 2) {
	float temp = 25.0f + temp_data * (1.0f / 16.0f);
	Serial.print("TEMP: ");
	Serial.println(temp);
      }
    }
    if (status_reg & 0x2) {
      // gyroscope data available
      if (readBytes(OUTX_L_G, dataBuffer.buffer, 6) == 6) {
	// Got gyro data
	Serial.print("GYRO: ");
	Serial.print(dataBuffer.xyz.x);
	Serial.print(", ");
	Serial.print(dataBuffer.xyz.y);
	Serial.print(", ");
	Serial.println(dataBuffer.xyz.z);
      }
    }
    if (status_reg & 0x4) {
      // accel data available
      if (readBytes(OUTX_L_XL, dataBuffer.buffer, 6) == 6) {
	// Got gyro data
	Serial.print("ACCEL: ");
	Serial.print(dataBuffer.xyz.x);
	Serial.print(", ");
	Serial.print(dataBuffer.xyz.y);
	Serial.print(", ");
	Serial.println(dataBuffer.xyz.z);
      }
      delay(100);
    }
  }
};

LSM6DS3H motion;

#else  // V2

#include <NXPMotionSense.h>

NXPMotionSense imu;
NXPSensorFusion filter;

// Motion tracking. The NXPmotionsense library can supposedly do
// full absolute motion tracking, but currently we're only using
// the raw values from accelerometers and gyroscopes.
class Orientation : Looper {
  public:
  Orientation() : Looper(), accel_(0.0f, 0.0f, 0.0f) {
  }

  void Setup() override {
    imu.begin();
    // filter.begin(100);
  }

  void Loop() override {
    print_ |= monitor.ShouldPrint(Monitoring::MonitorSwings);

    if (imu.available()) {
      Vec3 accel, gyro, magnetic;

      // Read the motion sensors
      imu.readMotionSensor(accel.x, accel.y, accel.z,
                           gyro.x, gyro.y, gyro.z,
                           magnetic.x, magnetic.y, magnetic.z);
      // Clash detection
      //Serial.print("ACCEL2: ");
      //Serial.println((accel_ - accel).len2());
      if ( (accel_ - accel).len2() > 1.0) {
        // Needs de-bouncing
        saber.Clash();
      }
      accel_ = accel;

      // static float last_speed = 0.0;
      float speed = sqrt(gyro.z * gyro.z + gyro.y * gyro.y);

      if (print_) {
        Serial.print("Speed: yz: ");
        Serial.print(speed);
        Serial.print("x: ");
        Serial.println(gyro.x);
	print_ = false;
      }

      if (saber.IsOn()) {
	SaberBase::DoXMotion(gyro.x);
	SaberBase::DoMotion(speed);
      }
#ifdef ENABLE_AUDIO
      // TODO: Use SaberBase()
      // saber_synth.volume_.set_target(32768 * (0.5 + clamp(speed/200.0, 0.0, 0.5)));

      // TODO: speed delta?
      // saber_synth.AdjustDelta(speed);
#endif
      
      // last_speed = speed;
#if 0
      // Update the SensorFusion filter
      filter.update(accel.x, accel.y, accel.z,
                    gyro.x, gyro.y, gyro.z,
                    magnetic.x, magnetic.y, magnetic.z);

      // print the heading, pitch and roll
      float roll = filter.getRoll();
      float pitch = filter.getPitch();
      float heading = filter.getYaw();
      Serial.print("Orientation: ");
      Serial.print(heading);
      Serial.print(" ");
      Serial.print(pitch);
      Serial.print(" ");
      Serial.println(roll);
#endif
    }
  }
private:
  bool print_= false;
  Vec3 accel_;
};

Orientation orientation;
#endif   // V2
#endif   // ENABLE_MOTION


#ifdef ENABLE_AUDIO
// Turns off amplifier when no audio is played.
// Maybe name this IdleHelper or something instead??
class Amplifier : Looper, StateMachine {
public:
  Amplifier() : Looper() {}
protected:
  void Setup() override {
    // Audio setup
    delay(50);             // time for DAC voltage stable
    pinMode(amplifierPin, OUTPUT);
    delay(10);
    dac.SetStream(&dynamic_mixer);
    dynamic_mixer.streams_[0] = &audio_splicer;
    dynamic_mixer.streams_[1] = &beeper;
    for (size_t i = 0; i < NELEM(wav_players); i++) {
      dynamic_mixer.streams_[i+2] = wav_players + i;
    }
  }

  bool Active() {
//    if (saber_synth.on_) return true;
    if (audio_splicer.isPlaying()) return true;
    if (beeper.isPlaying()) return true;
    for (size_t i = 0; i < NELEM(wav_players); i++)
      if (wav_players[i].isPlaying())
	return true;
    return false;
  }

  void Loop() override {
    STATE_MACHINE_BEGIN();
    while (true) {
      while (Active()) YIELD();
      SLEEP(20);
     if (Active()) continue;
      Serial.println("Amplifier off.");
      digitalWrite(amplifierPin, LOW); // turn the amplifier off
      while (!Active()) YIELD();
    }
    STATE_MACHINE_END();
  }
};

Amplifier amplifier;
#endif

void setup() {
  //  delay(1000);
  Serial.begin(9600);
#ifdef ENABLE_SERIALFLASH
  SerialFlashChip::begin(6);
#endif
#ifdef ENABLE_SD
  if (!SD.begin(sdCardSelectPin)) {
    Serial.println("No sdcard found.");
  } else {
    Serial.println("Sdcard found..");
  }
#endif
  // Time to identify the blade.
  saber.FindBlade();
  Looper::DoSetup();
  SaberBase::DoBoot();
}

#ifdef ENABLE_WATCHDOG
void startup_early_hook() {
  WDOG_TOVALL = 1000; // The next 2 lines sets the time-out value. This is the value that the watchdog timer compares itself to
  WDOG_TOVALH = 0;
  WDOG_PRESC = 0; // prescaler
  WDOG_STCTRLH = (WDOG_STCTRLH_ALLOWUPDATE | WDOG_STCTRLH_WDOGEN); // Enable WDG
}

class WatchDog : Looper {
  void Loop() override {
    if (watchdogTimer_ > 5) {
      watchdogTimer_ = 0;
      
      noInterrupts();
      WDOG_REFRESH = 0xA602;
      WDOG_REFRESH = 0xB480;
      interrupts();
    }
  };

  elapsedMillis watchdogTimer_;
};
#endif

int last_activity = millis();

void loop() {
  Looper::DoLoop();

#ifdef ENABLE_SNOOZE
  bool on = false;
  BladeBase::IsOn(&on);
  if (
#ifdef ENABLE_AUDIO
    !saber_synth.on_ &&
      !playFlashRaw1.isPlaying() &&
      !playSdWav1.isPlaying() &&
      digitalRead(amplifierPin) == LOW &&
#endif
    !Serial && !on) {
    if (millis() - last_activity > 1000) {
      Serial.println("Snoozing...");
      Snooze.sleep(snooze_config);
    }
  } else {
    last_activity = millis();
  }
#endif
}
