#ifndef STYLES_RAINBOW_H
#define STYLES_RAINBOW_H

// Usage: Rainbow
// return value: COLOR
// Basic RGB rainbow.

class Rainbow {
public:
  void run(BladeBase* base) {
    m = millis();
    m = 1023 - (millis() & 0x3ff);
  }
  OverDriveColor getColor(int led) {
    Color16 c(std::max(0, (sin_table[((m * 3 + led * 50)) & 0x3ff] << 2)),
              std::max(0, (sin_table[((m * 3 + led * 50 + 1024 / 3)) & 0x3ff] << 2)),
              std::max(0, (sin_table[((m * 3 + led * 50 + 1024 * 2 / 3)) & 0x3ff] << 2)));
    OverDriveColor ret;
    ret.c = c;
    ret.overdrive = false;
    return ret;
  }
private:
  uint32_t m;
};

// Usage: RainbowRev
// return value: COLOR
// Basic RGB rainbow but flowing from the hiltoutward
class RainbowRev {
public:
  void run(BladeBase* base) {
    m = 1023 - (millis() & 0x3ff);
  }
  OverDriveColor getColor(int led) {
    Color16 c(std::max(0, (sin_table[((m * 3 + led * 50)) & 0x3ff] << 2)),
              std::max(0, (sin_table[((m * 3 + led * 50 + 1024 / 3)) & 0x3ff] << 2)),
              std::max(0, (sin_table[((m * 3 + led * 50 + 1024 * 2 / 3)) & 0x3ff] << 2)));
    OverDriveColor ret;
    ret.c = c;
    ret.overdrive = false;
    return ret;
  }
private:
  uint32_t m;
};


#endif
