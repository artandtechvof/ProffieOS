#ifndef FUNCTIONS_WAV_TIME_H
#define FUNCTIONS_WAV_TIME_H

#include "wav_effect.h"
#include "int.h"
#include "../blades/blade_base.h"

// Usage: WavLen<EFFECT_NAME>  
// EFFECT_NAMENUM : any off the available effect form the style edittor
// Returns length of wav file returned as const int in [ms]
// returned value: INTEGER
//
template<BladeEffectType NAME>
class WavLen {
 public:
  void run(BladeBase* blade) {
    found_ = false;
	eff_name_ = wav_effect.GetEffectName(NAME);
	for (size_t i = 0; i < NELEM(wav_players); i++) {
	    if(strstr(eff_name_, wav_players[i].effectname()) && wav_players[i].isPlaying()) {found_ = true;idx=(int)i;}
		if(eff_name_=="pstoff") {if(strstr(eff_name_, wav_players[i].effect2ndname()) && wav_players[i].isPlaying()) {found_ = true;idx=(int)i;}}
			if (found_) { value_ = clampi32(wav_players[idx].length()*1000, 0, 32767);}
			if (found_ && !shown_){
				STDOUT << "WavLen: eff_name:'" << eff_name_  << "', effectname: '";
				if(eff_name_=="pstoff") { STDOUT << wav_players[i].effect2ndname();} else{ STDOUT << wav_players[i].effectname();}
   				STDOUT << "' wav_player:" << i 
				       << ", filename=" << wav_players[i].filename() 
					   << ", length: " << wav_players[i].length() << "[s]," 
				       << " converted = " << value_ << "[ms]\n";
			shown_ = true;
			}
	}
	if (!found_){
		//should we retain value after the effect has gone? Only reset it when new effect is selected?
        value_ = 32767;
        shown_ = false;
	}
  }
  int getInteger(int led) { return value_; }
  int value_ = 0;
private:
  int idx;
  const char* eff_name_;
  //char *found_;
  bool found_;
  bool shown_;  
  WavEffect wav_effect;
  
};


// Usage: WavPct<EFFECT_NAME, NUM>  
// EFFECT_NAMENUM : any off the available effect form the style edittor
// NUM value 0..100 (or even more than 100%, for multiplication of wav length time) for 
// Returns percentage of length of wav file returned as const int in [ms]
// exmpl: If your out.wav file is 4.35s long WavPct<EFFECT_IGNITION,25> would return 25% of 4.35s * 1000ms = 1087 [ms]
// returned value: INTEGER
//
template<BladeEffectType NAME, int DEVISOR>
class WavPct {
 public:
  void run(BladeBase* blade) {
    found_ = false;
	eff_name_ = wav_effect.GetEffectName(NAME);
	for (size_t i = 0; i < NELEM(wav_players); i++) {
	    if(strstr(eff_name_, wav_players[i].effectname()) && wav_players[i].isPlaying()) {found_ = true; idx=(int)i;}
		if(eff_name_=="pstoff") {if(strstr(eff_name_, wav_players[i].effect2ndname()) && wav_players[i].isPlaying()) {found_ = true;idx=(int)i;}}
		if (found_) {value_ = clampi32(wav_players[idx].length()*1000*float(DEVISOR)/100.0f, 0, 32767);}
		if (found_ && !shown_){
			STDOUT << "WavPct: eff_name:'" << eff_name_  << "', effectname: '";
			if(eff_name_=="pstoff") { STDOUT << wav_players[i].effect2ndname();} else{ STDOUT << wav_players[i].effectname();}
			STDOUT << "' wav_player:" << i 
				   << ", filename=" << wav_players[i].filename() 
				   << ", length: " << wav_players[i].length() << "[s]," 
				   << " <" << DEVISOR <<">% = " << value_ << "[ms]\n";
		shown_ = true;
		}
	}
	if (!found_){
	 // for (size_t i = 0; i < NELEM(wav_players); i++) {
		// if(eff_name_=="pstoff" && wav_players[i].isPlaying() ) STDOUT << wav_players[i].filename() << ", " << wav_players[i].effect2ndname() << " -- pstoff -- not found \n";
	 // }	
		//should we retain value after the effect has gone? Only reset it when new effect is selected?
		//value_ = 0; // make it large so that a InOutHelper does not start with 0 and instantly ignites
        value_ = 32767;
		shown_ = false;
	}
  }
  int getInteger(int led) { return value_; }
  int value_ = 0;
private:
  int idx;
  const char* eff_name_;
  //char *found_;
  bool found_;
  bool shown_;  
  WavEffect wav_effect;
  
};

// Usage: WavPos<EFFECT_NAME>  
// EFFECT_NAME : any off the available effect form the style edittor
// Returns current position [0..WavLen] of wav file returned as const int in [ms]
// returned value: INTEGER
//
template<BladeEffectType NAME>
class WavPos {
 public:
  void run(BladeBase* blade) {
    m = millis();
	found_ = false;
	eff_name_ = wav_effect.GetEffectName(NAME);
	for (size_t i = 0; i < NELEM(wav_players); i++) {
	    if(strstr(eff_name_, wav_players[i].effectname()) && wav_players[i].isPlaying()) {found_ = true; shown_=false;idx=(int)i;};
		if(eff_name_=="pstoff") {if(strstr(eff_name_, wav_players[i].effect2ndname()) && wav_players[i].isPlaying()) {found_ = true; shown_=false;idx=(int)i;}}
			if(found_) { value_ = clampi32(wav_players[idx].pos()*1000, 0, 32767);}
			if (found_ && !shown_ && ((m % 100)==0)){
				STDOUT << "WavPos: eff_name:'" << eff_name_  << "', effectname: ";
				if(eff_name_==" pstoff") { STDOUT << wav_players[i].effect2ndname();} else{ STDOUT << wav_players[i].effectname();}
   				STDOUT << "' wav_player:" << i 
				       << ", filename=" << wav_players[i].filename() 
					   << ", length: " << wav_players[i].length() << "[s]," 
				       << " current pos = " << value_ << "[ms]\n";
			shown_ = true;
			}
	}
	if (!found_){
		value_ = 0; // make it 0 so that a pos does start with 0 when sound is not playing.
        shown_ = false;
	}
  }
  int getInteger(int led) { return value_; }
  int value_;
private:
  int idx;
  const char* eff_name_;
  //char *found_;
  bool found_;
  bool shown_;  
  WavEffect wav_effect;
  uint32_t m;
 
};


// Usage: WavRem<EFFECT_NAME>  
// EFFECT_NAME : any off the available effect form the style edittor
// Returns remaining time [WavLen..0] of wav file returned as const int in [ms]
// returned value: INTEGER
//
template<BladeEffectType NAME>
class WavRem {
 public:
  void run(BladeBase* blade) {
    m = millis();
   found_ = false;
	eff_name_ = wav_effect.GetEffectName(NAME);
	for (size_t i = 0; i < NELEM(wav_players); i++) {
	    if(strstr(eff_name_, wav_players[i].effectname()) && wav_players[i].isPlaying()) {found_ = true; shown_ = false;idx=(int)i;} ;
		if(eff_name_=="pstoff") {if(strstr(eff_name_, wav_players[i].effect2ndname()) && wav_players[i].isPlaying()) {found_ = true; shown_=false;idx=(int)i;}}
			if (found_) { value_ = clampi32((wav_players[idx].length()-wav_players[idx].pos())*1000, 0, 32767);}
			if (found_ && !shown_ && ((m % 100)==0)){
				STDOUT << "WavRem: eff_name:'" << eff_name_  << ", effectname: ";
				if(eff_name_=="pstoff") { STDOUT << wav_players[i].effect2ndname();} else{ STDOUT << wav_players[i].effectname();}
   				STDOUT << "' wav_player:" << i 
				       << ", filename=" << wav_players[i].filename() 
					   << ", length: " << wav_players[i].length() << "[s]," 
				       << " remaining time = " << value_ << "[ms]\n";
			shown_ = true;
			}
	}
	if (!found_){
		value_ = 32767; // make it large so that a InOutHelper does not start with 0 and instantly ignites
        shown_ = false;
	}
  }
  int getInteger(int led) { return value_; }
  int value_;
private:
  int idx;
  const char* eff_name_;
  //char *found_;
  bool found_;
  bool shown_;  
  WavEffect wav_effect;
  uint32_t m;
  
};

#endif
