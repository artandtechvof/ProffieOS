// This config file contains all the things should be the same for all
// V3 teensysaber configurations. In most cases there should be no need
// to change anything in this file.

#define VERSION_MAJOR 3
#define VERSION_MINOR 0
#define V2
#define V3
#define USE_I2S
#define GYRO_CLASS MPU6050 // external motion sensor

#define ORIENTATION ORIENTATION_TOP_TOWARDS_BLADE //specific for aat setup

// Teensy 3.2 pin map:
// A lot of these can be changed, but be careful, because:
//   o The pins used by the prop shield cannot be easily changed.
//   o Pins that are going to control normal LEDs (not neopixels) need PWM capability,
//     and not all teensy pins can do PWM.
//   o Touch input is not available on all pins.
//   o Sdcard chip select pin depends on what shield you use.
//   o Battery level and blade identification needs analog input, which is not possible
//     on all pins.
//
// See the teensy 3.2 pinout diagram for more info: https://www.pjrc.com/teensy/pinout.html
enum SaberPins {
  // Bottom edge (in pin-out diagram)
  sdCardSelectPin = 0,            // (See File->Example->SD->ReadWrite for
                                  // other possible values.)
  amplifierPin = 1,               // Amplifier enable pin (TeensySaber V2)
  motionSensorInterruptPin = 2,   // motion sensor interrupt (TeensySaber V2)
  bladePowerPin2 = 3,             // R- Optional power control (TeensySaber V2)
  bladePowerPin3 = 4,             // G- Optional power control (TeensySaber V2)
  bladePowerPin4 = 5,             // B- Optional power control (TeensySaber V2)
  bladePowerPin5 = 6,             // W- blade power control
  spiLedSelect = -1,              // APA102/dotstar chip select
  //spiLedDataOut = 7,			  // APA102/dotstar data.
  //spiLedClock = 8,			  // APA102/dotstar chip clck	
  blade2Pin = 7,				  // Aditional Neopixel output 
  blade3Pin = 8,				  // Aditional Neopixel output
  bclkPin = 9,                    // BCLK (digital audio)
  bladePowerPin1 = 10,            // blade power control -> not used in
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
  bladePin = 20,                  // Main blade Neopixel data, blade control, either WS2811 or PWM
  bladeIdentifyPin = 20,          // blade identify input / FoC
  bladePowerPin6 = 21,            // blade power control
  txd0Pin = 22,                   // TXD0 (digital audio)
  lrclkPin = 23,                  // LRCLK (digital audio)
};

#ifndef TEENSYDUINO
#error Please select your Teensy model from Tools->Board
#endif


// Teensy 3.2	pinnr	count
// gnd				-1	1
// SD CS (0)		0	2
// Amp Enable (1)	1	3
// INTin (2)		2	4
// R (3)			3	5
// G (4)			4	6
// B (5)			5	7
// W (6)			6	8
// A (7)			7	9
// B (8)			8	10
// BCLK (9)			9	11
// spare(10)		10	12
// DOUT SPI (11)	11	13
// DIN SPI (12)		12	14
// SCK (13)			13	15
// batt level (14)	14	16
// SW2 (15)			15	17
// SW1 (16)			16	18
// spare (17)		17	19
// SDA (18) 100 ohm	18	20
// SCL (19) 100 ohm	19	21
// WS2812 Data (20)	20	22
// spare (21)		21	23
// DOUT I2S (22)	22	24
// LRC (23)			23	25
// 3.3v out			24	26
// 5Vin				26	27



