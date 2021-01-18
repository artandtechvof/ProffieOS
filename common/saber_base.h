#ifndef COMMON_SABER_BASE_H
#define COMMON_SABER_BASE_H

#include "linked_list.h"
#include "vec3.h"

// SaberBase is our main class for distributing saber-related events, such
// as on/off/clash/etc. to where they need to go. Each SABERFUN below
// has a corresponding SaberBase::Do* function which invokes that function
// on all active SaberBases.
class SaberBase;
extern SaberBase* saberbases;


// An effect is a one-shot trigger for something
// Effects may have a location, but do not generally have a "duration".
// However, durations may be derived from blade styles or sound effect durations
// associated with the effect.
#define DEFINE_ALL_EFFECTS()			\
    DEFINE_EFFECT(NONE)				\
    DEFINE_EFFECT(CLASH)			\
    DEFINE_EFFECT(BLAST)			\
    DEFINE_EFFECT(FORCE)			\
    DEFINE_EFFECT(STAB)				\
    DEFINE_EFFECT(BOOT)				\
    DEFINE_EFFECT(LOCKUP_BEGIN)			\
    DEFINE_EFFECT(LOCKUP_END)			\
    DEFINE_EFFECT(DRAG_BEGIN)			\
    DEFINE_EFFECT(DRAG_END)			\
    DEFINE_EFFECT(PREON)			\
    DEFINE_EFFECT(IGNITION)			\
    DEFINE_EFFECT(RETRACTION)			\
    DEFINE_EFFECT(PSTOFF)			\
    DEFINE_EFFECT(CHANGE)			\
    DEFINE_EFFECT(NEWFONT)			\
    DEFINE_EFFECT(LOW_BATTERY)			\
    DEFINE_EFFECT(POWERSAVE)                    \
    DEFINE_EFFECT(BATTERY_LEVEL)                \
    DEFINE_EFFECT(FAST_ON)                      \
    /* Blaster effects */                       \
    DEFINE_EFFECT(STUN)				\
    DEFINE_EFFECT(FIRE)				\
    DEFINE_EFFECT(CLIP_IN)			\
    DEFINE_EFFECT(CLIP_OUT)			\
    DEFINE_EFFECT(RELOAD)			\
    DEFINE_EFFECT(MODE)				\
    DEFINE_EFFECT(RANGE)			\
    DEFINE_EFFECT(EMPTY)			\
    DEFINE_EFFECT(FULL)				\
    DEFINE_EFFECT(JAM)				\
    DEFINE_EFFECT(UNJAM)			\
    DEFINE_EFFECT(PLI_ON)			\
    DEFINE_EFFECT(PLI_OFF)                      \
    /* user-definable effects */                \
    DEFINE_EFFECT(USER1)			\
    DEFINE_EFFECT(USER2)			\
    DEFINE_EFFECT(USER3)			\
    DEFINE_EFFECT(USER4)			\
    DEFINE_EFFECT(USER5)


#define DEFINE_EFFECT(X) EFFECT_##X,
enum class EffectType {
  DEFINE_ALL_EFFECTS()
};
enum class EffectTypeHelper {
  DEFINE_ALL_EFFECTS()
  NUMBER_OF_EFFECTS
};
#undef DEFINE_EFFECT

#define DEFINE_EFFECT(X) constexpr EffectType EFFECT_##X=EffectType::EFFECT_##X;
DEFINE_ALL_EFFECTS();

// This can be used if we need to allocate an array with one entry per effect
constexpr size_t NUMBER_OF_EFFECTS = (size_t)EffectTypeHelper::NUMBER_OF_EFFECTS;
#undef DEFINE_EFFECT

class SaberBase {
protected:
  void Link(SaberBase* x) {
	//STDOUT.println("SaberBase::Link()"); // Note to self, you can not put a STDOUT here, it will freeze the ProffieOS
    CHECK_LL(SaberBase, saberbases, next_saber_);
    noInterrupts();
    x->next_saber_ = saberbases;
    saberbases = x;
    interrupts();
    CHECK_LL(SaberBase, saberbases, next_saber_);
  }
  void Unlink(const SaberBase* x) {
    CHECK_LL(SaberBase, saberbases, next_saber_);
    for (SaberBase** i = &saberbases; *i; i = &(*i)->next_saber_) {
      if (*i == x) {
        *i = x->next_saber_;
        CHECK_LL(SaberBase, saberbases, next_saber_);
        return;
      }
    }
    CHECK_LL(SaberBase, saberbases, next_saber_);
  }

  SaberBase() { Link(this); }
  explicit SaberBase(NoLink _) {}
  ~SaberBase() { Unlink(this); }

public:
  enum OffType {
    OFF_NORMAL,
    OFF_BLAST,
    OFF_IDLE,
  };

  static bool IsOn() { return on_; }
  static void TurnOn() {
    on_ = true;
    SaberBase::DoOn();
  }
  static void TurnOff(OffType off_type) {
    on_ = false;
    last_motion_request_ = millis();
    SaberBase::DoOff(off_type);
  }

  static bool MotionRequested() {
#if NUM_BUTTONS == 0
    return true;
#else
    return IsOn() || (millis() - last_motion_request_) < 20000;
#endif
  }
  static void RequestMotion() {
    last_motion_request_ = millis();
  }

  enum LockupType {
    LOCKUP_NONE,
    LOCKUP_NORMAL,
    LOCKUP_DRAG,
    LOCKUP_ARMED,   // For detonators and such
    LOCKUP_AUTOFIRE, // For blasters and phasers
    LOCKUP_MELT,     // For cutting through doors...
    LOCKUP_LIGHTNING_BLOCK,  // Lightning block lockup
  };
  static LockupType Lockup() { return lockup_; }
  static void SetLockup(LockupType lockup) { lockup_ = lockup; }

  enum ChangeType {
    ENTER_COLOR_CHANGE,
    EXIT_COLOR_CHANGE,
    CHANGE_COLOR,
	ENTER_VOLUME_CHANGE,
	ENTER_DIM_CHANGE,
	EXIT_MENU
  };

  // 1.0 = kDefaultVolume
  // This is really just for sound fonts.
  virtual void SetHumVolume(float volume) {}
  virtual void StartSwing(const Vec3& gyro, float swingThreshold, float slashThreshold) {}
  virtual float SetSwingVolume(float swing_strength, float mixhum) {
    return mixhum;
  }

#define SABERFUN(NAME, TYPED_ARGS, ARGS)                        \
public:                                                         \
  static void Do##NAME TYPED_ARGS {                             \
    CHECK_LL(SaberBase, saberbases, next_saber_);               \
    for (SaberBase *p = saberbases; p; p = p->next_saber_) {    \
      p->SB_##NAME ARGS;                                        \
    }                                                           \
    CHECK_LL(SaberBase, saberbases, next_saber_);               \
  }                                                             \
                                                                \
  virtual void SB_##NAME TYPED_ARGS {}

#define SABERBASEFUNCTIONS()						\
  SABERFUN(Effect, (EffectType effect, float location), (effect, location)); \
  SABERFUN(PreOn, (float* delay), (delay));				\
  SABERFUN(On, (), ());                  				\
  SABERFUN(Off, (OffType off_type), (off_type));			\
  SABERFUN(BladeDetect, (bool detected), (detected));			\
  SABERFUN(Change, (ChangeType change_type), (change_type));		\
									\
  SABERFUN(Top, (uint64_t total_cycles), (total_cycles));		\
  SABERFUN(IsOn, (bool* on), (on));					\
  SABERFUN(Message, (const char* msg), (msg));				\

  
  SABERBASEFUNCTIONS();

  static void DoEffectR(EffectType e) { DoEffect(e, (200 + random(700))/1000.0f); }
  static void DoClash() { DoEffectR(EFFECT_CLASH); }
  static void DoBlast() { DoEffectR(EFFECT_BLAST); }
  static void DoForce() { DoEffectR(EFFECT_FORCE); }
  static void DoStab() { DoEffect(EFFECT_STAB, 1.0f); }
  static void DoBoot() { DoEffect(EFFECT_BOOT, 0); }
  static void DoBeginLockup() { DoEffectR(EFFECT_LOCKUP_BEGIN); }
  static void DoEndLockup() { DoEffect(EFFECT_LOCKUP_END, 0); }
  static void DoChange() { DoEffect(EFFECT_CHANGE, 0); }
  static void DoNewFont() { DoEffect(EFFECT_NEWFONT, 0); }
  static void DoLowBatt() { DoEffect(EFFECT_LOW_BATTERY, 0); }
  
  
#undef SABERFUN

  /* Swing rotation speed degrees per second */
  static void DoMotion(Vec3 gyro, bool clear) {
#ifdef INVERT_ORIENTATION
    gyro.x = -gyro.x;
#endif
    CHECK_LL(SaberBase, saberbases, next_saber_);
    for (SaberBase *p = saberbases; p; p = p->next_saber_) {
      p->SB_Motion(gyro, clear);
    }
    CHECK_LL(SaberBase, saberbases, next_saber_);
  }
  virtual void SB_Motion(const Vec3& gyro, bool clear) {}

  /* Acceleration in g */
  static void DoAccel(Vec3 gyro, bool clear) {
#ifdef INVERT_ORIENTATION
    gyro.x = -gyro.x;
#endif
    CHECK_LL(SaberBase, saberbases, next_saber_);
	//int nr_of_saberbases = 0;
    for (SaberBase *p = saberbases; p; p = p->next_saber_) {
      p->SB_Accel(gyro, clear);
	//  nr_of_saberbases++;
    }
	//STDOUT.print("nr_of_saberbases :");
	//STDOUT.println(nr_of_saberbases);
    CHECK_LL(SaberBase, saberbases, next_saber_);
  }
  virtual void SB_Accel(const Vec3& gyro, bool clear) {}

  static uint32_t GetCurrentVariation() {
    return current_variation_;
  }

  static uint32_t GetCurrentBrightness() {
    return current_brightness_;//value from 0~16384
  }
  // For smooth updates or restore.
  static void SetBrightness(float v) {
	current_brightness_ = (int)(v * 16384 / 100.0);  
    current_brightness_ = clampi32(current_brightness_, 0, 16384) ;
  }


  // For step-wise updates
  static void UpdateVariation(int delta) {
    current_variation_ += delta;
    DoChange(CHANGE_COLOR);
  }
  // For smooth updates or restore.
  static void SetVariation(uint32_t v) {
    current_variation_ = v;
  }

  enum ColorChangeMode {
    COLOR_CHANGE_MODE_NONE,
    COLOR_CHANGE_MODE_STEPPED,
    COLOR_CHANGE_MODE_SMOOTH
  };

  static ColorChangeMode GetColorChangeMode() { return color_change_mode_; }
  static void SetColorChangeMode(ColorChangeMode  mode) {
    color_change_mode_ = mode;
    if (mode == COLOR_CHANGE_MODE_NONE) {
      //DoChange(COLOR_COLOR_CHANGE); // to prevent beeps from playing
    } else {
      DoChange(ENTER_COLOR_CHANGE);
    }
  }

  enum VolumeChangeMode {
    VOLUME_CHANGE_MODE_NONE,
    VOLUME_CHANGE_MODE_SMOOTH
  };

  static VolumeChangeMode GetVolumeChangeMode() { return volume_change_mode_; }
  static void SetVolumeChangeMode(VolumeChangeMode  mode) {
    volume_change_mode_ = mode;
    if (mode == VOLUME_CHANGE_MODE_NONE) {
      //DoChange(VOLUME_COLOR_CHANGE); // to prevent beeps from playing
    } else {
      DoChange(ENTER_VOLUME_CHANGE);
    }
  }

  enum DimChangeMode {
    DIM_CHANGE_MODE_NONE,
    DIM_CHANGE_MODE_SMOOTH
  };

  static DimChangeMode GetDimChangeMode() { return dim_change_mode_; }
  static void SetDimChangeMode(DimChangeMode  mode) {
    dim_change_mode_ = mode;
    if (mode == DIM_CHANGE_MODE_NONE) {
      //DoChange(VOLUME_COLOR_CHANGE); // to prevent beeps from playing
    } else {
      DoChange(ENTER_DIM_CHANGE);
    }
  }

  enum MenuChangeMode {
    MENU_CHANGE_MODE_NONE,
    MENU_CHANGE_MODE_ACTIVE
  };

  static MenuChangeMode GetMenuChangeMode() { return menu_change_mode_; }
  static void SetMenuChangeMode(MenuChangeMode mode) {
    menu_change_mode_ = mode;
    if (mode == MENU_CHANGE_MODE_NONE) {
      DoChange(EXIT_MENU);
    }
  }


private:
  static bool on_;
  static LockupType lockup_;
  static uint32_t last_motion_request_;
  static uint32_t current_variation_;
  static uint32_t current_brightness_;
  static ColorChangeMode color_change_mode_;
  static VolumeChangeMode volume_change_mode_;
  static DimChangeMode dim_change_mode_;
  static MenuChangeMode menu_change_mode_;
  SaberBase* next_saber_;
};

#endif
