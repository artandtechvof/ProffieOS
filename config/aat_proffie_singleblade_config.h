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
#include "proffieboard_v2_config.h"
#define NUM_BLADES 1
#define NUM_BUTTONS 1
#define VOLUME 50
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
// #define KEEP_SAVEFILES_WHEN_PROGRAMMING
// Must be 20 characters or less.
// #define BLE_PASSWORD "password"
// FASTLED is experimental and untested right now
// #define ENABLE_FASTLED
// #define ENABLE_WATCHDOG
// #define ENABLE_SERIALFLASH
// #define ENABLE_SSD1306

// #define ENABLE_DEBUG

   #define IDLE_OFF_TIME 100000  // if defined, pulsing glow will not work,it stops after 100 seconds
#endif

#ifdef CONFIG_PRESETS
Preset presets[] = {
  { "common;VaderII", "tracks/main.wav",
	StylePtr<Layers<
	  Red,
	  BlastL<White>,
	  LockupL<AudioFlicker<Red,White>>,
	  SimpleClashL<White>,
	  InOutHelperL<InOutFuncX<OutWavTime,InWavTime>>
	 >>(),
	"blue"},
  { "common;Kanan", "tracks/main.wav",
	StylePtr<Layers<
	  Blue,
	  BlastL<White>,
	  LockupL<AudioFlicker<Blue,White>>,
	  SimpleClashL<White>,
	  InOutHelperL<InOutFuncX<OutWavTime,InWavTime>>
	>>(),
    "layers"},  
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
    StyleFirePtr<BLUE, CYAN, 0>(),
	"blue fire"},
  { "common;Electric", "tracks/cantina.wav",
	StylePtr<InOutHelper<Lockup<StyleFire<Blast<LocalizedClash<Gradient<Red,OrangeRed,Orange,GreenYellow,Green,Aqua,DodgerBlue,Blue,Rgb<20,0,255>>,White,200,80>,White,300,200,400>,Red,10,2,FireConfig<100,2000,5>,FireConfig<2000,500,50>,FireConfig<0,5000,10>,FireConfig<100,2000,5>>,Pulsing<StyleFire<Blast<LocalizedClash<Gradient<Red,Orange,Strobe<White,RandomPerLEDFlicker<Blue,White>,50,1>,Green,Aqua,DodgerBlue,Blue,Rgb<20,0,255>>,White,200,80>,White,300,200,400>,Red,10,2,FireConfig<100,2000,5>,FireConfig<2000,500,50>,FireConfig<0,5000,10>,FireConfig<100,2000,5>>,StyleFire<Blast<LocalizedClash<Gradient<Red,Orange,Strobe<White,RandomPerLEDFlicker<Blue,White>,50,1>,Green,DodgerBlue,Blue,Rgb<20,0,255>>,White,200,80>,White,300,200,400>,Red,10,2,FireConfig<100,2000,5>,FireConfig<2000,500,50>,FireConfig<0,5000,10>,FireConfig<100,2000,5>>,3500>>,150,800,Black>>(),
	"rainbow new style"}
};
BladeConfig blades[] = {
 { 0, WS281XBladePtr<48, bladePin, Color8::GRB, PowerPINS<bladePowerPin1> >(),
    CONFIGARRAY(presets) },
};

#endif

#ifdef CONFIG_BUTTONS
Button PowerButton(BUTTON_POWER, powerButtonPin, "pow");
//ButtonPullDown AuxButton(BUTTON_AUX, auxPin, "aux");
//PullDownButton Aux2Button(BUTTON_AUX2, aux2Pin, "aux2");
//PullDownButton UpButton(BUTTON_UP, blade4Pin, "aux3");
#endif


//1 button setup
//Sketch uses 201136 bytes (81%) of program storage space. Maximum is 245760 bytes.
//Sketch uses 201144 bytes (81%) of program storage space. Maximum is 245760 bytes.  -> 8 bytes extra, with option pull down...

//4-button setup
//Proffieboard without Pulldown option:  Sketch uses 201328 bytes (81%) of program storage space. Maximum is 245760 bytes.
//Proffieboard with Pulldown mod:        Sketch uses 201384 bytes (81%) of program storage space. Maximum is 245760 bytes.  -> 56 additional bytes when option are left default
//Proffieboard with Pulldown mod:        Sketch uses 201392 bytes (81%) of program storage space. Maximum is 245760 bytes.  -> 64 additional bytes when option are set true/false/true/false


//Teensy 4 button button

//Sketch uses 185136 bytes (70%) of program storage space. Maximum is 262144 bytes.
//Global variables use 29904 bytes (45%) of dynamic memory, leaving 35632 bytes for local variables. Maximum is 65536 bytes.

// true/false/true/false
// Sketch uses 185200 bytes (70%) of program storage space. Maximum is 262144 bytes.
// Global variables use 29904 bytes (45%) of dynamic memory, leaving 35632 bytes for local variables. Maximum is 65536 bytes.


//single button
//no fifference Sketch uses 201256 bytes (81%) of program storage space. Maximum is 245760 bytes.

//2 button setup
// original: Sketch uses 201368 bytes (81%) of program storage space. Maximum is 245760 bytes.
// 2x ButtonPullDown Sketch uses 201368 bytes (81%) of program storage space. Maximum is 245760 bytes. -> exact same
// 1x Button, 1x ButtonPullDown Sketch uses 201480 bytes (81%) of program storage space. Maximum is 245760 bytes. -> 112 bytes extra

//4button setup 
//original, 												  Sketch uses 201456 bytes (81%) of program storage space. Maximum is 245760 bytes.

//Using additional class ButtonPullDown
// 4 buttons original class									  Sketch uses 201456 bytes (81%) of program storage space. Maximum is 245760 bytes. total bytes unchanged
// 4 buttons all ButtonPullDown type						  Sketch uses 201448 bytes (81%) of program storage space. Maximum is 245760 bytes.  -> 8 bytes less (!)
// only 1 button uses ButtonPullDown class			          Sketch uses 201632 bytes (82%) of program storage space. Maximum is 245760 bytes.  -> 176 extra
// 2x Buttons, 2x ButtonPullDown   							  Sketch uses 201680 bytes (82%) of program storage space. Maximum is 245760 bytes. -> 224 bytes extra
// 3 buttons use ButtonPullDown type 						  Sketch uses 201632 bytes (82%) of program storage space. Maximum is 245760 bytes. -> 176 bytes extra
