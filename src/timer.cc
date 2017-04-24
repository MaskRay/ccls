#include "timer.h"

#include <iostream>

Timer::Timer() {
  Reset();
}

void Timer::Reset() {
  start = Clock::now();
}

void Timer::ResetAndPrint(const std::string& message) {
  std::chrono::time_point<Clock> end = Clock::now();
  long long elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

  long long milliseconds = elapsed / 1000;
  long long remaining = elapsed - milliseconds;

  std::cerr << message << " took " << milliseconds << "." << remaining << "ms" << std::endl;
  Reset();
}