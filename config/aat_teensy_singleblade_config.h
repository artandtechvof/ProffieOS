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
#define NUM_BLADES 1
#define NUM_BUTTONS 1
#define VOLUME 100
#define MAXVOLUME 2500
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
//#define KEEP_SAVEFILES_WHEN_PROGRAMMING
// Must be 20 characters or less.
// #define BLE_PASSWORD "password"
// FASTLED is experimental and untested right now
// #define ENABLE_FASTLED
// #define ENABLE_WATCHDOG
// #define ENABLE_SERIALFLASH
// #define ENABLE_SSD1306
#define NO_BATTERY_MONITOR  //AaT teensy sabers do not have battery monitoring (yet)

// #define ENABLE_DEBUG

   #define IDLE_OFF_TIME 100000  // if defined, pulsing glow will not work,it stops after 100 seconds
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
  { "common;Kanan", "tracks/main.wav",
	StylePtr<Layers<
	  Blue,
	  BlastL<White>,
	  LockupL<AudioFlicker<Blue,White>>,
	  SimpleClashL<White>,
	  InOutHelperL<InOutFuncX< Percentage<WavLen<EFFECT_IGNITION>,50>,WavLen<EFFECT_RETRACTION> >>
	>>(),
    "layers kanan pct"},  
  { "common;Kanan", "tracks/main.wav",
	StylePtr<Layers<
	  Blue,
	  BlastL<White>,
	  LockupL<AudioFlicker<Blue,White>>,
	  SimpleClashL<White>,
	  InOutHelperL<InOutFuncX< WavLen<EFFECT_IGNITION>,WavLen<EFFECT_RETRACTION> >>
	>>(),
    "layers kanan"},  
  { "common;Dark", "tracks/main.wav",
	// StylePtr<Layers<
	  // Blue,
	  // BlastL<White>,
	  // LockupL<AudioFlicker<Blue,White>>,
	  // SimpleClashL<White>,
	  // InOutHelperL<InOutFuncX< Scale<WavPos<EFFECT_IGNITION>, Int<150>, Int<65535>>, WavRem<EFFECT_RETRACTION>  >>
	// >>(),
	StylePtr<
	  Layers<
			Blue,
			BlastL<White>,
			LockupL<AudioFlicker<Blue,White>>,
			TransitionEffectL<TrConcat<TrInstant,AlphaL<White,Bump<Int<16384>>>,TrFadeX<WavLen<EFFECT_CLASH>>>,EFFECT_CLASH>,
			InOutTrL<TrDelayX<WavLen<EFFECT_IGNITION>>,TrInstant>, //Make sure Transition Effect has enough time to do its thing
			TransitionEffectL<TrConcat<TrInstant,AlphaL<Blue,SmoothStep< Scale<Mult<Mult<WavRem<EFFECT_IGNITION>,WavRem<EFFECT_IGNITION>>,WavRem<EFFECT_IGNITION>>,Int<32768>,Int<0> >,Int<-1>> >,TrDelayX<WavLen<EFFECT_IGNITION>>>,EFFECT_IGNITION>, //position controller extension
			TransitionEffectL<TrConcat<TrInstant,AlphaL<Blue,SmoothStep< Scale<Mult<Mult<WavPos<EFFECT_RETRACTION>,WavPos<EFFECT_RETRACTION>>,WavPos<EFFECT_RETRACTION>>,Int<32768>,Int<0> >,Int<-1>> >,TrDelayX<WavLen<EFFECT_RETRACTION>>>,EFFECT_RETRACTION> //position controller retraction
		    >
	>(),
    "layers progressive dark"},  
  { "common;SmthJedi", "tracks/rebels.wav",
    StylePtr<InOutHelper<EasyBlade<OnSpark<Green, White, 555>, White>, 300, 800> >(),
	"green"},
  { "common;TeensySF", "tracks/impmarch.wav",
    StyleNormalPtr<Red, BLUE, 300, 800>(),
	"red"},
  { "common;Fire", "tracks/dueloffates.wav",
    StyleFirePtr<RED, YELLOW, 0>(),
	"fire"},
  { "common;ArtnTech", "tracks/muse.wav",
	// StylePtr<Layers<
      // StyleFire<Blue,Cyan,0,2,FireConfig<0,2000,5>,FireConfig<3000,0,0>,FireConfig<0,5000,10>,FireConfig<0,0,10>>,
	  // SimpleClashL<White>,
	  // ResponsiveLockupL<Strobe<White,BrownNoiseFlicker<White,Red,300>,50,1>,TrConcat<TrInstant,White,TrFade<200>>,TrFade<400>,Scale<BladeAngle<0,16000>,Int<10000>,Int<30000>>,Int<10000>,Scale<SwingSpeed<100>,Int<10000>,Int<14000>>>,
  	  // ResponsiveDragL<AudioFlicker<Blue,White>,TrInstant,TrInstant,Int<10000>,Int<16384>>,
      // ResponsiveMeltL<Mix<TwistAngle<>,Red,Orange>,TrWipeIn<600>,TrSmoothFade<600>>,
	  // ResponsiveStabL<Orange,TrWipeIn<50>,TrWipe<50>>,BlastL<White>,
	  // InOutTrL<TrWipe<300>,TrWipeIn<500>>,
 	  // TransitionEffectL<TrConcat<TrInstant,AlphaL<AudioFlicker<Cyan,Blue>,Bump<Int<0>,Int<24000>>>,TrFade<600>>,EFFECT_RETRACTION>
      // >>(),
	StylePtr<Layers<
	  StyleFire<Blue,Cyan,0,2,FireConfig<0,2000,5>,FireConfig<3000,0,0>,FireConfig<0,5000,10>,FireConfig<0,2000,5>>,
	  BlastL<White>,
	  LockupL<Pulsing<
		Gradient<StyleFire<Blue,Cyan,0,2,FireConfig<0,2000,5>,FireConfig<3000,0,0>,FireConfig<0,5000,10>,FireConfig<0,2000,5>>,StyleFire<Blue,Cyan,0,2,FireConfig<0,2000,5>,FireConfig<3000,0,0>,FireConfig<0,5000,10>,FireConfig<0,2000,5>>,BrownNoiseFlicker<White,Strobe<Red,White,50,1>,100>,StyleFire<Blue,Cyan,0,2,FireConfig<0,2000,5>,FireConfig<3000,0,0>,FireConfig<0,5000,10>,FireConfig<0,2000,5>>,StyleFire<Blue,Cyan,0,2,FireConfig<0,2000,5>,FireConfig<3000,0,0>,FireConfig<0,5000,10>,FireConfig<0,2000,5>>,StyleFire<Blue,Cyan,0,2,FireConfig<0,2000,5>,FireConfig<3000,0,0>,FireConfig<0,5000,10>,FireConfig<0,2000,5>>>,
		Gradient<StyleFire<Blue,Cyan,0,2,FireConfig<0,2000,5>,FireConfig<3000,0,0>,FireConfig<0,5000,10>,FireConfig<0,2000,5>>,StyleFire<Blue,Cyan,0,2,FireConfig<0,2000,5>,FireConfig<3000,0,0>,FireConfig<0,5000,10>,FireConfig<0,2000,5>>,BrownNoiseFlicker<White,Strobe<Red,White,50,1>,100>,StyleFire<Blue,Cyan,0,2,FireConfig<0,2000,5>,FireConfig<3000,0,0>,FireConfig<0,5000,10>,FireConfig<0,2000,5>>,StyleFire<Blue,Cyan,0,2,FireConfig<0,2000,5>,FireConfig<3000,0,0>,FireConfig<0,5000,10>,FireConfig<0,2000,5>>>,3500>,StyleFire<Blue,Cyan,0,2,FireConfig<0,2000,5>,FireConfig<3000,0,0>,FireConfig<0,5000,10>,FireConfig<0,2000,5>>>,
	  ResponsiveDragL<AudioFlicker<Blue,White>,TrInstant,TrInstant,Int<10000>,Int<16384>>,
	  SimpleClashL<White>,
	  ResponsiveStabL<Orange,TrWipeIn<50>,TrWipe<50>>,BlastL<White>,
	  ResponsiveMeltL<Mix<TwistAngle<>,Red,Orange>,TrWipeIn<600>,TrSmoothFade<600>>,
	  InOutHelperL<InOutFuncX<Int<180>,Int<500>>>>>(),
	  "blue fire"},
//  { "common;TthCrstl", "tracks/cantina.wav",
//	StyleRainbowRevPtrX<WavLen<EFFECT_IGNITION>,WavLen<EFFECT_RETRACTION>>(), //rainbow flowing from hilt
//	"rainbow blade"},
  { "common;Electric", "tracks/cantina.wav",
//	StylePtr<InOutHelper<Lockup<StyleFire<Blast<LocalizedClash<Gradient<Red,OrangeRed,Orange,GreenYellow,Green,Aqua,DodgerBlue,Blue,Rgb<20,0,255>>,White,200,80>,White,300,200,400>,Red,10,2,FireConfig<100,2000,5>,FireConfig<2000,500,50>,FireConfig<0,5000,10>,FireConfig<100,2000,5>>,Pulsing<StyleFire<Blast<LocalizedClash<Gradient<Red,Orange,Strobe<White,RandomPerLEDFlicker<Blue,White>,50,1>,Green,Aqua,DodgerBlue,Blue,Rgb<20,0,255>>,White,200,80>,White,300,200,400>,Red,10,2,FireConfig<100,2000,5>,FireConfig<2000,500,50>,FireConfig<0,5000,10>,FireConfig<100,2000,5>>,StyleFire<Blast<LocalizedClash<Gradient<Red,Orange,Strobe<White,RandomPerLEDFlicker<Blue,White>,50,1>,Green,DodgerBlue,Blue,Rgb<20,0,255>>,White,200,80>,White,300,200,400>,Red,10,2,FireConfig<100,2000,5>,FireConfig<2000,500,50>,FireConfig<0,5000,10>,FireConfig<100,2000,5>>,3500>>,150,800,Black>>(),
	StylePtr<Layers<
	  StyleFire<Blast<LocalizedClash<Gradient<Red,OrangeRed,Orange,GreenYellow,Green,Cyan,DodgerBlue,Blue,Rgb<20,0,255>>,White,200,80>,White,300,200>,Red,10,2,FireConfig<100,2000,5>,FireConfig<2000,500,50>,FireConfig<0,5000,10>,FireConfig<100,2000,5>>,
	  LockupL<Pulsing<StyleFire<Blast<LocalizedClash<Gradient<Red,Orange,Strobe<White,RandomPerLEDFlicker<Blue,White>,50,1>,Green,Cyan,DodgerBlue,Blue,Rgb<20,0,255>>,White,200,80>,White,300,200>,Red,10,2,FireConfig<100,2000,5>,FireConfig<2000,500,50>,FireConfig<0,5000,10>,FireConfig<100,2000,5>>,StyleFire<Blast<LocalizedClash<Gradient<Red,Orange,Strobe<White,RandomPerLEDFlicker<Blue,White>,50,1>,Green,DodgerBlue,Blue,Rgb<20,0,255>>,White,200,80>,White,300,200>,Red,10,2,FireConfig<100,2000,5>,FireConfig<2000,500,50>,FireConfig<0,5000,10>,FireConfig<100,2000,5>>,3500>>,
	  InOutHelperL<InOutFuncX<Int<150>,Int<800>>>>>(),
	"rainbow new style"},
  // { "common;ArtnTech", "tracks/muse.wav",
    // StylePtr<LengthFinder<>>(),  // to chekc how many pixel an insetred blade has, usage = select preset and rotate hilt, or serial monitor -> var = num pixels -1
	// "length finder"},
};
BladeConfig blades[] = {
 { 0,  DimBlade(100.0,WS281XBladePtr<123, bladePin, Color8::GRB, PowerPINS<> >()), //set initial blade brightness to 100%
    CONFIGARRAY(presets) },
};

#endif

#ifdef CONFIG_BUTTONS
Button PowerButton(BUTTON_POWER, powerButtonPin, "pow");
//Button AuxButton(BUTTON_AUX, auxPin, "aux");
//Button Aux2Button(BUTTON_AUX2, aux2Pin, "aux2");
//Button UpButton(BUTTON_UP, bladePowerPin4, "aux3");
#endif