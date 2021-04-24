#ifndef COMMON_STDOUT_H
#define COMMON_STDOUT_H

#include "monitoring.h"
#ifdef TEENSYDUINO
#include "_usb_rawhid.h"
#endif

extern Print* default_output;
extern Print* stdout_output;


template<typename T, typename X = void> struct PrintHelper {
  static void out(Print& p, T& x) { p.print(x); }
};

template<typename T> struct PrintHelper<T, decltype(((T*)0)->printTo(*(Print*)0))> {
  static void out(Print& p, T& x) { x.printTo(p); }
};


class ConsoleHelper : public Print {
public:
  bool debug_is_on() const {
    return monitor.IsMonitoring(Monitoring::MonitorSerial) &&
      stdout_output != default_output;
  }
  size_t write(uint8_t b) override {
#ifdef TEENSYDUINO
#if defined(RAWHID_INTERFACE)	
	size_t ret = 1;
    if (stdout_output) {
		ret = stdout_output->write(b);
	} else {
		SerialHID.write(b);   
	}
#else
	size_t	ret = stdout_output->write(b);
#endif
#else
	size_t	ret = stdout_output->write(b);
#endif
    if (debug_is_on()) default_output->write(b);
    return ret;
  }
  size_t write(const uint8_t *buffer, size_t size) override {
#ifdef TEENSYDUINO
#if defined(RAWHID_INTERFACE)	
	size_t ret = 1;
    if (stdout_output) {
		ret = stdout_output->write(buffer, size);
	} else {
		SerialHID.write(buffer, size);   
	}
#else
	size_t	ret = stdout_output->write(buffer, size);
#endif
#else
	size_t	ret = stdout_output->write(buffer, size);
#endif
    if (debug_is_on()) default_output->write(buffer, size);
    return ret;
  }
  template<typename T>
  ConsoleHelper& operator<<(T v) {
    PrintHelper<T>::out(*this, v);
    return *this;
  }

#ifdef TEENSYDUINO
  int availableForWrite(void) override {
    return stdout_output->availableForWrite();
  }
  virtual void flush() override {
    stdout_output->flush();
    if (debug_is_on()) default_output->flush();
  }
#endif
};

extern ConsoleHelper STDOUT;

#define DEFINE_COMMON_STDOUT_GLOBALS            \
Print* default_output = &Serial;                \
Print* stdout_output = &Serial;                 \
ConsoleHelper STDOUT

#endif
