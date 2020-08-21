// This config file contains all the things should be the same for all
// V2 proffiboard configurations. In most cases there should be no need
// to change anything in this file.

#define VERSION_MAJOR 5
#define VERSION_MINOR 1
#define V2
#define V3
#define PROFFIEBOARD
#define USE_I2S
#define GYRO_CLASS LSM6DS3H

// Proffieboard pin map  , 5V toleracny according to DS11449 Rev 5 STM32L433xx FT = tolerant, TT is 3.6 tolerant
enum SaberPins {
  // I2S
  bclkPin = 3,                   // BCLK (digital audio)   PB13 FT_fl
  txd0Pin = 26,                  // TXD0 (digital audio)   PA10 FT_fl
  lrclkPin = 2,                  // LRCLK (digital audio)  PB12 FT_I

  // I2C
  i2cDataPin = 7,                 // I2C bus, Used by motion sensors  PB9 FT_fl
  i2cClockPin = 25,               // I2C bus, Used by motion sensors  PA9 FT_fl

  // Buttons
  powerButtonPin = 21,            // power button  PB6 FT_I
  auxPin = 23,                    // AUX button    PB5 FT_l
  aux2Pin = 22,                   // AUX2 button   PB4 FT_fla

  // Memory card
  sdCardSelectPin = 4,            // PB14 FT_fl
  // No fastled support yet
  spiLedSelect = -1,
  spiLedDataOut =-1,
  spiLedClock = -1,
  amplifierPin = 24,              // Amplifier enable pin PH1 FT
  boosterPin = 15,                // Booster enable pin   PH0 FT
  motionSensorInterruptPin = 12,  // motion sensor interrupt PC13 FT


  // Neopixel pins
  bladePin = 16,                  // blade control, either WS2811 or PWM PA0  FT_a
  bladeIdentifyPin = 16,          // blade identify input / FoC  
  blade2Pin = 1,                  // PB10   FT_fl
  blade3Pin = 17,                 // PA4  -> not 5V tolerant  TT_a
  blade4Pin = 0,                  // PB3  FT_la	
  blade5Pin = 8,                  // PA15 (also UART)  FT_li
  blade6Pin = 9,                  // PA02 (also UART)  FT_la

  // Blade power control
  bladePowerPin1 = 13,            // blade power control PA1  FT_la
  bladePowerPin2 = 19,            // blade power control PB8  FT_fl
  bladePowerPin3 = 18,            // blade power control PA8  FT_l
  bladePowerPin4 = 10,            // blade power control PB3  FT_la
  bladePowerPin5 = 5,             // blade power control PB15 FT_I
  bladePowerPin6 = 6,             // blade power control PB0  FT_la

  // If there is no neopixels, these can be used as PWM output
  // If neopixels are present, but not on these pins, PWM may still
  // be possible at 800kHz.
  // You can drive 20mA LEDs with a resistor from these pins, or
  // hook up an external FET to drive more powerful LEDs
  bladePowerPin7 = 17,            // PB03 (also neopix4)
  bladePowerPin8 = 1,             // PB10 (also neopix2)
  bladePowerPin9 = 0,             // PB11 (also neopix3)
  bladePowerPin10 = 16,           // PA00 (also blade ID / neopixels)

  // Analog pins
  batteryLevelPin = 14,           // battery level input PA04 TT_a  -> Not 5V tolerant, not broken out to any pads on proffieboard

  // UART
  rxPin = 8,                      // PA15  FT_I
  txPin = 9,                      // PA02  FT_la
};

#if PROFFIEBOARD_VERSION - 0 != 2
#error Please select Proffieboard V2 in Tools->Board
#endif
#if DOSFS_SDCARD - 0 == 0
#error Tools->DOSFS should be set to SDCARD (SPI)
#endif
