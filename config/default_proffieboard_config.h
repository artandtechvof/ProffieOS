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
#define NUM_BLADES 2
#define NUM_BUTTONS 2
#define VOLUME 100
const unsigned int maxLedsPerStrip = 144;
#define CLASH_THRESHOLD_G 1.0
#define ENABLE_AUDIO
#define ENABLE_MOTION
#define ENABLE_WS2811
#define ENABLE_SD
#define ENABLE_SERIAL
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
    StyleNormalPtr<Green, White, 500, 1800>(),
	"layers vader"},	

	  // { "common;Portal", "tracks/main.wav",
     // StylePtr<Layers<
		 // Blue,
		 // BlastL<White>,
		 // SimpleClashL<White>,
		 // ResponsiveLockupL<Strobe<White,BrownNoiseFlicker<White,Red,300>,50,1>,
		                   // TrConcat<TrInstant,White,TrFade<200>>,TrFade<400>,
						            // Scale<BladeAngle<0,16000>,Int<10000>,Int<30000>>,Int<10000>,
									// Scale<SwingSpeed<100>,Int<10000>,Int<14000>>>,
		 // ResponsiveDragL<AudioFlicker<Blue,White>,TrInstant,TrInstant,Int<10000>,Int<16384>>,
		 // ResponsiveMeltL<Mix<TwistAngle<>,Red,Orange>,TrWipeIn<600>,TrSmoothFade<600>>,
		 // ResponsiveStabL<Orange,TrWipeIn<50>,TrWipe<50>>,BlastL<White>,
		 // ResponsiveLightningBlockL<White>,
		 // InOutTrL<  TrWipeX< Percentage<WavLen<EFFECT_IGNITION>,10> >, TrWipeInX< Percentage<WavLen<EFFECT_RETRACTION>, 5> >,Black>,
		 
		 // TransitionEffectL<TrConcat<TrInstant,AlphaL<White,Bump<Int<0>,Int<6000>>>,
						   // TrFadeX<WavLen<EFFECT_RETRACTION>>,AlphaL<Orange,Bump<Int<0>,Int<4000>>>,
						   // TrFadeX< Percentage<WavLen<EFFECT_PSTOFF>,60> >,AlphaL<Red,Bump<Int<0>,Int<3000>>>,
						   // TrFadeX< Percentage<WavLen<EFFECT_PSTOFF>,40>> >,EFFECT_RETRACTION>,
						   
		 // TransitionEffectL<TrConcat<TrFadeX< Percentage<WavLen<EFFECT_PREON>,125> >,AlphaL<HumpFlickerL<Orange,10>,Bump<Int<0>,Int<4000>>>,
		                   // TrFadeX< Percentage<WavLen<EFFECT_IGNITION>,50> >,AlphaL<HumpFlickerL<Orange,15>,Bump<Int<0>,Int<5000>>>,
						   // TrFadeX< Percentage<WavLen<EFFECT_IGNITION>,25> >,AlphaL<HumpFlickerL<White,20>,Bump<Int<0>,Int<6000>>>,
						   // TrBoingX< Percentage<WavLen<EFFECT_IGNITION>,15>,3>>,EFFECT_PREON>>
		 // >(),	
    // StyleNormalPtr<Green, White, 500, 1800>(),
	// "layers vader"},	

    { "common;Portal", "tracks/main.wav",
	StylePtr<Layers<Blue, // basic base layer for now
		// whole rest of style goes here

		InOutTrL<
		// Extention - wipe out spark tip followed by 2 stage slowing stripes that swell from blue to orange then fade out
		TrJoin<
		   TrWipe<500>,
		   TrWaveX<LemonChiffon,Int<1000>,Int<130>,Int<500>,Int<0>>,
		   TrConcat<TrWipe<500>,AlphaL<Stripes<1400,-3600,Blue,Rgb<0,0,60>,Blue,Rgb<0,0,128>>,Int<32768>>,
					TrFade<350>,AlphaL<Stripes<1400,-3000,DarkOrange,Rgb<60,17,0>,Orange,Rgb<128,34,0>>,Int<32768>>,
					TrSmoothFade<600>>>,
		// Retraction - wipe in 3 stage accelerating stripes followed by spark in
		TrConcat<
		TrWipeInX< Percentage<WavLen<EFFECT_RETRACTION>,40> >,Stripes<3000,1500,Blue,Orange>,
		TrFadeX< Percentage<WavLen<EFFECT_RETRACTION>,20> >,        Stripes<3000,2000,Blue,Orange>,
		TrFadeX< Percentage<WavLen<EFFECT_RETRACTION>,20> >,        Stripes<3000,2500,Blue,Orange>,
		TrJoin<
		  TrWipeInX< Percentage<WavLen<EFFECT_RETRACTION>,20> >,
		  TrWaveX<LemonChiffon, Percentage<WavLen<EFFECT_RETRACTION>,100> ,Int<130>, Percentage<WavLen<EFFECT_RETRACTION>,20> ,Int<32768>> >>,
		// Color When Off
		Black>,

		// Ignition Effects
		TransitionEffectL<TrConcat<TrInstant,White,TrFade<50>>,EFFECT_IGNITION>,

		// Ignition Emitter flare
		TransitionEffectL<TrConcat<
		TrInstant,AlphaL<LemonChiffon,Bump<Int<0>,Int<10000>>>,
		TrFade<750>,AlphaL<Orange,Bump<Int<0>,Int<4000>>>,
		TrFade<150>,AlphaL<Rgb<255,54,0>,Bump<Int<0>,Int<2800>>>,
		TrFade<150>,AlphaL<OrangeRed,Bump<Int<0>,Int<1700>>>,
		TrFade<200>>,EFFECT_IGNITION>,

		// Retraction Emitter flare
		MultiTransitionEffectL<TrConcat<
		TrInstant,AlphaL<OrangeRed,Bump<Int<0>,Int<1000>>>,
		TrFadeX< Percentage<WavLen<EFFECT_RETRACTION>,20> >,AlphaL<Rgb<255,54,0>,Bump<Int<0>,Int<3500>>>,
		TrFadeX< Percentage<WavLen<EFFECT_RETRACTION>,20> >,AlphaL<Orange,Bump<Int<0>,Int<9000>>>,
		//TrFadeX< Percentage<WavLen<EFFECT_RETRACTION>,30> >,AlphaL<BrownNoiseFlicker<LemonChiffon,Black,300>,Bump<Int<0>,Int<17000>>>, //Optional effect
		TrFadeX< Percentage<WavLen<EFFECT_RETRACTION>,30> >,AlphaL<Strobe<LemonChiffon,Black,15,50>,Bump<Int<0>,Int<17000>>>,
		TrFadeX< Percentage<WavLen<EFFECT_RETRACTION>,30> >>,EFFECT_RETRACTION,4>,

		// pstoff Effects - first delay is wait for retraction retraction to finish
		MultiTransitionEffectL<TrConcat<TrDelayX< Percentage<WavLen<EFFECT_RETRACTION>,100> >,AlphaL<White,SmoothStep<Int<15000>,Int<-20000>>>,TrFade<75>>,EFFECT_RETRACTION,4>,
		MultiTransitionEffectL<TrConcat<TrDelayX< Percentage<WavLen<EFFECT_RETRACTION>,100> >,AlphaL<Red,Int<0>>,TrFadeX< Percentage<WavLen<EFFECT_RETRACTION>,70> >,SparkleL<EffectSequence<EFFECT_BLAST,Blue,Orange>>,TrSmoothFadeX< Percentage<WavLen<EFFECT_RETRACTION>,100> > >,EFFECT_RETRACTION,4>,
		MultiTransitionEffectL<TrConcat<
							  TrDelayX< Percentage<WavLen<EFFECT_RETRACTION>,100> >,AlphaL<Blue,Bump<Int<0>,Int<5000>>>,
							  TrDelayX< Percentage<WavLen<EFFECT_PSTOFF>,16>  >,AlphaL<Orange,Bump<Int<0>,Int<4000>>>,
							  TrDelayX< Percentage<WavLen<EFFECT_PSTOFF>,16>  >,AlphaL<Blue,Bump<Int<0>,Int<3000>>>,
							  TrDelayX< Percentage<WavLen<EFFECT_PSTOFF>,16>  >,AlphaL<Orange,Bump<Int<0>,Int<1600>>>,
							  TrDelayX< Percentage<WavLen<EFFECT_PSTOFF>,16>  >,AlphaL<Blue,Bump<Int<0>,Int<1000>>>,
							  TrDelayX< Percentage<WavLen<EFFECT_PSTOFF>,16>  >,AlphaL<Orange,Bump<Int<0>,Int<500>>>,
							  TrDelayX< Percentage<WavLen<EFFECT_PSTOFF>,16>  >>,EFFECT_RETRACTION,4>,
		// preon
		TransitionEffectL<TrConcat<
							  TrInstant,                                      AlphaL<Orange,Bump<Int<0>,Int<500>>>,
							  TrDelayX< Percentage<WavLen<EFFECT_PREON>,16> >,AlphaL<Blue,Bump<Int<0>,Int<1000>>>,
							  TrDelayX< Percentage<WavLen<EFFECT_PREON>,16> >,AlphaL<Orange,Bump<Int<0>,Int<2000>>>,
							  TrDelayX< Percentage<WavLen<EFFECT_PREON>,16> >,AlphaL<Blue,Bump<Int<0>,Int<3000>>>,
							  TrDelayX< Percentage<WavLen<EFFECT_PREON>,16> >,AlphaL<Orange,Bump<Int<0>,Int<4000>>>,
							  TrDelayX< Percentage<WavLen<EFFECT_PREON>,16> >,AlphaL<Blue,Bump<Int<0>,Int<5000>>>,
							  TrDelayX< Percentage<WavLen<EFFECT_PREON>,16> >>,EFFECT_PREON> >>(), 	
    StyleNormalPtr<Green, White, 500, 1800>(),
	"layers Portal"},	
	
	
	
  { "TeensySF", "tracks/venus.wav",
    StyleNormalPtr<CYAN, WHITE, 300, 800>(),
    StyleNormalPtr<CYAN, WHITE, 300, 800>(), "cyan"},
  { "SmthJedi", "tracks/mars.wav",
    StylePtr<InOutSparkTip<EASYBLADE(BLUE, WHITE), 300, 800> >(),
    StylePtr<InOutSparkTip<EASYBLADE(BLUE, WHITE), 300, 800> >(), "blue"},
  { "SmthGrey", "tracks/mercury.wav",
    StyleFirePtr<RED, YELLOW, 0>(),
    StyleFirePtr<RED, YELLOW, 1>(), "fire"},
  { "SmthFuzz", "tracks/uranus.wav",
    StyleNormalPtr<RED, WHITE, 300, 800>(),
    StyleNormalPtr<RED, WHITE, 300, 800>(), "red"},
  { "RgueCmdr", "tracks/venus.wav",
    StyleFirePtr<BLUE, CYAN, 0>(),
    StyleFirePtr<BLUE, CYAN, 1>(), "blue fire"},
  { "TthCrstl", "tracks/mars.wav",
    StylePtr<InOutHelper<EASYBLADE(OnSpark<GREEN>, WHITE), 300, 800> >(),
    StylePtr<InOutHelper<EASYBLADE(OnSpark<GREEN>, WHITE), 300, 800> >(), "green"},
  { "TeensySF", "tracks/mercury.wav",
    StyleNormalPtr<WHITE, RED, 300, 800, RED>(),
    StyleNormalPtr<WHITE, RED, 300, 800, RED>(), "white"},
  { "SmthJedi", "tracks/uranus.wav",
    StyleNormalPtr<AudioFlicker<YELLOW, WHITE>, BLUE, 300, 800>(),
    StyleNormalPtr<AudioFlicker<YELLOW, WHITE>, BLUE, 300, 800>(), "yellow"},
  { "SmthGrey", "tracks/venus.wav",
    StylePtr<InOutSparkTip<EASYBLADE(MAGENTA, WHITE), 300, 800> >(),
    StylePtr<InOutSparkTip<EASYBLADE(MAGENTA, WHITE), 300, 800> >(), "magenta"},
  { "SmthFuzz", "tracks/mars.wav",
    StyleNormalPtr<Gradient<RED, BLUE>, Gradient<CYAN, YELLOW>, 300, 800>(),
    StyleNormalPtr<Gradient<RED, BLUE>, Gradient<CYAN, YELLOW>, 300, 800>(), "gradient"},
  { "RgueCmdr", "tracks/mercury.wav",
    StyleRainbowPtr<300, 800>(),
    StyleRainbowPtr<300, 800>(), "rainbow"},
  { "TthCrstl", "tracks/uranus.wav",
    StyleStrobePtr<WHITE, Rainbow, 15, 300, 800>(),
    StyleStrobePtr<WHITE, Rainbow, 15, 300, 800>(), "strobe"},
  { "SmthJedi", "tracks/mars.wav",
    &style_charging,
    StyleNormalPtr<BLACK, BLACK, 300, 800>(), "Battery\nLevel"}
};
BladeConfig blades[] = {
 { 0, //WS2811BladePtr<97, WS2811_800kHz>(),
     WS281XBladePtr<50, bladePin, Color8::GRB, PowerPINS<> >(),
     SimpleBladePtr<CreeXPE2WhiteTemplate<550>,
                    CreeXPE2BlueTemplate<240>,
                    CreeXPE2BlueTemplate<240>, NoLED, bladePowerPin4, bladePowerPin5, bladePowerPin6, -1>(),
     CONFIGARRAY(presets) },
  };
#endif

#ifdef CONFIG_BUTTONS
Button PowerButton(BUTTON_POWER, powerButtonPin, "pow");
Button AuxButton(BUTTON_AUX, auxPin, "aux");
#endif
