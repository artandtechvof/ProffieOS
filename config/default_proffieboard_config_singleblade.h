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
#include "proffieboard_config.h"
#define NUM_BLADES 1
#define NUM_BUTTONS 1
#define VOLUME 100
#define MAXVOLUME 3000 //upper level of volume menu
const unsigned int maxLedsPerStrip = 144;
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
// #define KEEP_SAVEFILES_WHEN_PROGRAMMING
// Must be 20 characters or less.
// #define BLE_PASSWORD "password"
// FASTLED is experimental and untested right now
// #define ENABLE_FASTLED
// #define ENABLE_WATCHDOG
// #define ENABLE_SERIALFLASH
// #define ENABLE_SSD1306

// #define ENABLE_DEBUG
#endif

#ifdef CONFIG_PRESETS
Preset presets[] = {
  { "common;VaderII", "tracks/main.wav",
     StylePtr<Layers<
		 Blue,
		 BlastL<White>,
		 SimpleClashL<White>,
		 ResponsiveLockupL<Strobe<White,BrownNoiseFlicker<White,Red,300>,50,1>,
		                   TrConcat<TrInstant,White,TrFade<200>>,TrFade<400>,
						            Scale<BladeAngle<0,16000>,Int<10000>,Int<30000>>,Int<10000>,
									Scale<SwingSpeed<100>,Int<10000>,Int<14000>>>,
		 ResponsiveDragL<AudioFlicker<Blue,White>,TrInstant,TrInstant,Int<10000>,Int<16384>>,
		 ResponsiveMeltL<Mix<TwistAngle<>,Red,Orange>,TrWipeIn<600>,TrSmoothFade<600>>,
		 ResponsiveStabL<Orange,TrWipeIn<50>,TrWipe<50>>,BlastL<White>,
		 ResponsiveLightningBlockL<White>,
		 InOutTrL<  TrWipeX< Percentage<WavLen<EFFECT_IGNITION>,10> >, TrWipeInX< Percentage<WavLen<EFFECT_RETRACTION>, 5> >,Black>,
		 
		 TransitionEffectL<TrConcat<TrInstant,AlphaL<White,Bump<Int<0>,Int<6000>>>,
						   TrFadeX<WavLen<EFFECT_RETRACTION>>,AlphaL<Orange,Bump<Int<0>,Int<4000>>>,
						   TrFadeX< Percentage<WavLen<EFFECT_PSTOFF>,60> >,AlphaL<Red,Bump<Int<0>,Int<3000>>>,
						   TrFadeX< Percentage<WavLen<EFFECT_PSTOFF>,40>> >,EFFECT_RETRACTION>,
						   
		 TransitionEffectL<TrConcat<TrFadeX< Percentage<WavLen<EFFECT_PREON>,125> >,AlphaL<HumpFlickerL<Orange,10>,Bump<Int<0>,Int<4000>>>,
		                   TrFadeX< Percentage<WavLen<EFFECT_IGNITION>,50> >,AlphaL<HumpFlickerL<Orange,15>,Bump<Int<0>,Int<5000>>>,
						   TrFadeX< Percentage<WavLen<EFFECT_IGNITION>,25> >,AlphaL<HumpFlickerL<White,20>,Bump<Int<0>,Int<6000>>>,
						   TrBoingX< Percentage<WavLen<EFFECT_IGNITION>,15>,3>>,EFFECT_PREON>>
		 >(),	
	"layers vader"},	
  { "TeensySF", "tracks/venus.wav",
    StyleNormalPtr<CYAN, WHITE, 300, 800>(), "cyan"},
  { "SmthJedi", "tracks/mars.wav",
    StylePtr<InOutSparkTip<EASYBLADE(BLUE, WHITE), 300, 800> >(), "blue"},
  { "SmthGrey", "tracks/mercury.wav",
    StyleFirePtr<RED, YELLOW, 1>(), "fire"},
  { "SmthFuzz", "tracks/uranus.wav",
    StyleNormalPtr<RED, WHITE, 300, 800>(), "red"},
  { "RgueCmdr", "tracks/venus.wav",
    StyleFirePtr<BLUE, CYAN, 1>(), "blue fire"},
  { "TthCrstl", "tracks/mars.wav",
    StylePtr<InOutHelper<EASYBLADE(OnSpark<GREEN>, WHITE), 300, 800> >(), "green"},
  { "TeensySF", "tracks/mercury.wav",
    StyleNormalPtr<WHITE, RED, 300, 800, RED>(), "white"},
  { "SmthJedi", "tracks/uranus.wav",
    StyleNormalPtr<AudioFlicker<YELLOW, WHITE>, BLUE, 300, 800>(), "yellow"},
  { "SmthGrey", "tracks/venus.wav",
    StylePtr<InOutSparkTip<EASYBLADE(MAGENTA, WHITE), 300, 800> >(), "magenta"},
  { "SmthFuzz", "tracks/mars.wav",
    StyleNormalPtr<Gradient<RED, BLUE>, Gradient<CYAN, YELLOW>, 300, 800>(), "gradient"},
  { "RgueCmdr", "tracks/mercury.wav",
    StyleRainbowPtr<300, 800>(), "rainbow"},
  { "TthCrstl", "tracks/uranus.wav",
    StyleStrobePtr<WHITE, Rainbow, 15, 300, 800>(), "strobe"},
  { "SmthJedi", "tracks/mars.wav",
    &style_charging,
    "Battery\nLevel"}
};
BladeConfig blades[] = {
 { 0, DimBlade(75,WS281XBladePtr<123, bladePin, Color8::GRB, PowerPINS<> >()), //Start off at 75% brightness
     CONFIGARRAY(presets) },
  };
#endif

#ifdef CONFIG_BUTTONS
Button PowerButton(BUTTON_POWER, powerButtonPin, "pow");
//Button AuxButton(BUTTON_AUX, auxPin, "aux");
#endif
