#ifndef FUNCTIONS_WAV_TIME_H
#define FUNCTIONS_WAV_TIME_H

// Usage: InOutWavTime
// Returns 0-32768 time in [ms] of current in/out wav.
// returned value: INTEGER
class InOutWavTime {
 public:
  void run(BladeBase* blade) {
	if (hybrid_font.inout_player_){
		value_ = clampi32(hybrid_font.inout_player_->length()*1000, 0, 32768);
		// if (hybrid_font.inout_player_->isPlaying()){
			// STDOUT << "Current InOut pos(): " << hybrid_font.inout_player_->pos() << "\n";
		// }
	}else{
		value_ = 32767; // make it large so that a InOutHelper does not start with 0 and instantly ignites
	}	
  }	
  int getInteger(int led) { return value_; }
  int value_;
};


// Usage: InOutWavPrecentage<NUM>  NUM form 0..100 for precentage of length of wav file returned as const in [ms]
// (might also be larger than 100% to multiply the time)
// If your wav file is 4.35s long InOutWavPrecentage<25> would return 25% of 4.35s * 1000ms = 1087 [ms]
// Returns 0-32768 time in [ms]
// returned value: INTEGER
template<int DEVISOR>
class InOutWavPercentage {
 public:
  void run(BladeBase* blade) {
	if (hybrid_font.inout_player_){
		value_ = clampi32(hybrid_font.inout_player_->length()*1000*float(DEVISOR)/100.0f, 0, 32768);
		 //if (hybrid_font.inout_player_->isPlaying()){
		 //	 STDOUT << "Percentage InOut : " << int32_t(hybrid_font.inout_player_->pos()/hybrid_font.inout_player_->length()  * int32_t(100)) << "\n";
		 //}
	}else{
		value_ = 32767; // make it large so that a InOutHelper does not start with 0 and instantly ignites
	}	
  }	
  int getInteger(int led) { return value_; }
  int value_;
};


// Usage: InOutWavPos
// Returns 0-32768 time in [ms] of current time elapsed of in/out wav.
// returned value: INTEGER
class InOutWavPos {
 public:
  void run(BladeBase* blade) {
	if (hybrid_font.inout_player_){
		value_ = clampi32(hybrid_font.inout_player_->pos()*1000, 0, 32768);
		//if (hybrid_font.inout_player_->isPlaying()){
		//	STDOUT << "Current InOut pos(): " << value_ << "\n";
		//}
	}else{
		value_ = 32767;
	}	
  }	
  int getInteger(int led) { return value_; }
  int value_;
};

// Usage: InOutWavRem
// Returns 0-32768 time in [ms] of remaining time of in/out wav.
// returned value: INTEGER
class InOutWavRem {
 public:
  void run(BladeBase* blade) {
	if (hybrid_font.inout_player_){
		value_ = clampi32( (hybrid_font.inout_player_->length() -  hybrid_font.inout_player_->pos() )*1000, 0, 32768);
		//if (hybrid_font.inout_player_->isPlaying()){
		//	STDOUT << "remaining InOut time: " << value_ << "\n";
		//}
	}else{
		value_ = 32767;
	}	
  }	
  int getInteger(int led) { return value_; }
  int value_;
};


#endif
