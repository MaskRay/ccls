#include "timer.h"

#include <iostream>

Timer::Timer() {
  Reset();
}

void Timer::Reset() {
  start = Clock::now();
}

void Timer::ResetAndPrint(const std::string& message) {
  std::cerr << message << " took " << ElapsedMilliseconds() << "ms" << std::endl;
  Reset();
}

long long Timer::ElapsedMilliseconds() {
  std::chrono::time_point<Clock> end = Clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}
