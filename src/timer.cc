#include "timer.h"

Timer::Timer() {
  Reset();
}

void Timer::Reset() {
  start = Clock::now();
}

long long Timer::ElapsedMilliseconds() {
  std::chrono::time_point<Clock> end = Clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}
