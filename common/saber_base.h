#ifndef COMMON_SABER_BASE_H
#define COMMON_SABER_BASE_H

#include "linked_list.h"

// SaberBase is our main class for distributing saber-related events, such
// as on/off/clash/etc. to where they need to go. Each SABERFUN below
// has a corresponding SaberBase::Do* function which invokes that function
// on all active SaberBases.
class SaberBase;
extern SaberBase* saberbases;

class SaberBase {
protected:
  void Link(SaberBase* x) {
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
  enum LockupType1 {
    LOCKUP1_NONE,
    LOCKUP1_NORMAL,
    LOCKUP1_DRAG,
    LOCKUP1_ARMED,   // For detonators and such
    LOCKUP1_AUTOFIRE, // For blasters and phasers
    LOCKUP1_MELT,     // For cutting through doors...
    LOCKUP1_LIGHTNING_BLOCK,  // Lightning block lockup
  };
  static LockupType Lockup() { return lockup_; }
  static LockupType1 Lockup1() { return lockup1_; }
  static void SetLockup(LockupType lockup) { lockup_ = lockup; }
  static void SetLockup1(LockupType1 lockup1) { lockup1_ = lockup1; }
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

#define SABERBASEFUNCTIONS()					\
  SABERFUN(LowBatt, (), ());                     		\
  SABERFUN(Clash, (), ());					\
  SABERFUN(Stab, (), ());					\
  SABERFUN(PreOn, (float* delay), (delay));			\
  SABERFUN(On, (), ());						\
  SABERFUN(Off, (OffType off_type), (off_type));		\
  SABERFUN(Force, (), ());					\
  SABERFUN(Blast, (), ());					\
  SABERFUN(Boot, (), ());					\
  SABERFUN(BladeDetect, (bool detected), (detected));		\
  SABERFUN(NewFont, (), ());					\
  SABERFUN(BeginLockup, (), ());				\
  SABERFUN(EndLockup, (), ());					\
  SABERFUN(Change, (ChangeType change_type), (change_type));	\
								\
  SABERFUN(Top, (uint64_t total_cycles), (total_cycles));	\
  SABERFUN(Relax, (), ());					\
  SABERFUN(IsOn, (bool* on), (on));				\
  SABERFUN(Message, (const char* msg), (msg));  \
  \
  SABERFUN(Stun, (), ());          \
  SABERFUN(Fire, (), ());          \
  SABERFUN(ClipIn, (), ());          \
  SABERFUN(ClipOut, (), ());          \
  SABERFUN(Reload, (), ());          \
  SABERFUN(Mode, (), ());          \
  SABERFUN(Range, (), ());          \
  SABERFUN(Empty, (), ());          \
  SABERFUN(Full, (), ());          \
  SABERFUN(Jam, (), ());          \
  SABERFUN(UnJam, (), ());          \
  SABERFUN(PLIOn, (), ());          \
  SABERFUN(PLIOff, (), ());
  SABERBASEFUNCTIONS();
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
    for (SaberBase *p = saberbases; p; p = p->next_saber_) {
      p->SB_Accel(gyro, clear);
    }
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
  static LockupType1 lockup1_;
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
