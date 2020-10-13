// This is a sample configuration file.
// This saber has:
//   o Proffieboard V1 hardware.
//   o Two buttons
//   o An XP-E2 RGB LED star on LED 4-5-6
//   o A neopixel blade on LED 1-2-3 and neopixel data 1.
// If you have a saber similar to this one, make a copy and use the copy.
// This is also the default configuration file. Pre-programmed boards will
// use this configuration file.

#ifdef CONFIG_TOP
#include "aat_teensy_hardware_v3_config.h"
#define NUM_BLADES 2
#define NUM_BUTTONS 2
#define VOLUME 50
#define MAXVOLUME 1500
const unsigned int maxLedsPerStrip = 97;
#define CLASH_THRESHOLD_G 1.0
#define ENABLE_AUDIO
#define ENABLE_MOTION
#define ENABLE_WS2811
#define ENABLE_SD
#define ENABLE_SERIAL
#define ENABLE_DEVELOPER_COMMANDS
// #define DISABLE_DIAGNOSTIC_COMMANDS
#define SAVE_COLOR_CHANGE
// #define DISABLE_COLOR_CHANGE
#define SAVE_STATE
// Must be 20 characters or less.
// #define BLE_PASSWORD "password"
// FASTLED is experimental and untested right now
// #define ENABLE_FASTLED
// #define ENABLE_WATCHDOG
// #define ENABLE_SERIALFLASH
// #define ENABLE_SSD1306

// #define ENABLE_DEBUG

// #define IDLE_OFF_TIME 100000, if defined, pulsing glow will not work
#endif

#ifdef CONFIG_PRESETS
Preset presets[] = {
  { "TeensySF", "tracks/venus.wav",
    StyleNormalPtr<YELLOW, BLUE, 300, 800>(),
    StyleNormalPtr<RED, BLUE, 300, 800>(),
//    StylePtr<InOutHelper<Blinking<Red, Black, 55,55>, 1, 1, Pulsing<Red, Black, 5555>>>(),
//    StylePtr<InOutHelper<Red, 1, 1, Pulsing<Black, Red, 5555>>>(),
//    StylePtr<InOutHelper<Blinking<Red, Black, 77,77>, 1, 1, Pulsing<Red, Black, 5555>>>(),
//    StylePtr<InOutHelper<Red, 1, 1, Pulsing<Black, Red, 5555>>>(),
	"red"},
  { "SmthJedi", "tracks/mars.wav",
    StylePtr<InOutHelper<EasyBlade<OnSpark<Blue, White, 555>, White>, 300, 800> >(),
	StylePtr<IgnitionDelay<555, RetractionDelay<555, InOutHelper<EasyBlade<OnSpark<Blue, White, 555>, White>, 300, 800>>>>(),
//    StylePtr<InOutHelper<Blinking<Red, Black, 55,55>, 1, 1, Pulsing<Red, Black, 5555>>>(),
//    StylePtr<InOutHelper<Red, 1, 1, Pulsing<Black, Red, 5555>>>(),
//    StylePtr<InOutHelper<Blinking<Red, Black, 77,77>, 1, 1, Pulsing<Red, Black, 5555>>>(),
//    StylePtr<InOutHelper<Red, 1, 1, Pulsing<Black, Red, 5555>>>(),
	"blue"},
  { "TeensySF", "tracks/venus.wav",
    StyleNormalPtr<Green, BLUE, 300, 800>(),
    StyleNormalPtr<Green, BLUE, 300, 800>(),
//    StylePtr<InOutHelper<Blinking<Red, Black, 55,55>, 1, 1, Pulsing<Red, Black, 5555>>>(),
//    StylePtr<InOutHelper<Red, 1, 1, Pulsing<Black, Red, 5555>>>(),
//    StylePtr<InOutHelper<Blinking<Red, Black, 77,77>, 1, 1, Pulsing<Red, Black, 5555>>>(),
//    StylePtr<InOutHelper<Red, 1, 1, Pulsing<Black, Red, 5555>>>(),
	"red"},
  { "SmthGrey", "tracks/mercury.wav",
    StyleFirePtr<RED, YELLOW, 0>(),
    StyleFirePtr<RED, YELLOW, 1>(), 
//    StylePtr<InOutHelper<Blinking<Red, Black, 55,55>, 1, 1, Pulsing<Red, Black, 5555>>>(),
//    StylePtr<InOutHelper<Red, 1, 1, Pulsing<Black, Red, 5555>>>(),
//    StylePtr<InOutHelper<Blinking<Red, Black, 77,77>, 1, 1, Pulsing<Red, Black, 5555>>>(),
//    StylePtr<InOutHelper<Red, 1, 1, Pulsing<Black, Red, 5555>>>(),
	"fire"},
  { "RgueCmdr", "tracks/venus.wav",
    StyleFirePtr<BLUE, CYAN, 0>(),
    StyleFirePtr<BLUE, CYAN, 1>(), 
//    StylePtr<InOutHelper<Blinking<Red, Black, 55,55>, 1, 1, Pulsing<Red, Black, 5555>>>(),
//    StylePtr<InOutHelper<Red, 1, 1, Pulsing<Black, Red, 5555>>>(),
//    StylePtr<InOutHelper<Blinking<Red, Black, 77,77>, 1, 1, Pulsing<Red, Black, 5555>>>(),
//    StylePtr<InOutHelper<Red, 1, 1, Pulsing<Black, Red, 5555>>>(),
	"blue fire"},
  { "RgueCmdr", "tracks/mercury.wav",
    StyleRainbowPtr<300, 800>(),
    StyleRainbowPtr<300, 800>(), 
//    StylePtr<InOutHelper<Blinking<Red, Black, 55,55>, 1, 1, Pulsing<Red, Black, 5555>>>(),
//    StylePtr<InOutHelper<Red, 1, 1, Pulsing<Black, Red, 5555>>>(),
//    StylePtr<InOutHelper<Blinking<Red, Black, 77,77>, 1, 1, Pulsing<Red, Black, 5555>>>(),
//    StylePtr<InOutHelper<Red, 1, 1, Pulsing<Black, Red, 5555>>>(),
	"rainbow"}
};
BladeConfig blades[] = {
 { 0, WS281XBladePtr<90, bladePin, Color8::GRB, PowerPINS<bladePowerPin1> >(),
    WS281XBladePtr<80, blade2Pin, Color8::GRB, PowerPINS<bladePowerPin6> >(),
//    SimpleBladePtr<CH1LED, NoLED, NoLED, NoLED, bladePowerPin2, -1, -1, -1>(),
//    SimpleBladePtr<CH1LED, NoLED, NoLED, NoLED, bladePowerPin3, -1, -1, -1>(),
//    SimpleBladePtr<CH1LED, NoLED, NoLED, NoLED, bladePowerPin4, -1, -1, -1>(),
//    SimpleBladePtr<CH1LED, NoLED, NoLED, NoLED, bladePowerPin5, -1, -1, -1>(),
    CONFIGARRAY(presets) },
};

#endif

#ifdef CONFIG_BUTTONS
Button PowerButton(BUTTON_POWER, powerButtonPin, "pow");
Button AuxButton(BUTTON_AUX, auxPin, "aux");
#endif
