#include "timer.h"

#include <loguru.hpp>

#include <iostream>

Timer::Timer() {
  Reset();
}

long long Timer::ElapsedMicroseconds() const {
  std::chrono::time_point<Clock> end = Clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(end - start)
      .count();
}

long long Timer::ElapsedMicrosecondsAndReset() {
  std::chrono::time_point<Clock> end = Clock::now();
  long long microseconds =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();
  Reset();
  return microseconds;
}

void Timer::Reset() {
  start = Clock::now();
}

void Timer::ResetAndPrint(const std::string& message) {
  std::chrono::time_point<Clock> end = Clock::now();
  long long elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();

  long long milliseconds = elapsed / 1000;
  long long remaining = elapsed - milliseconds;

  LOG_S(INFO) << message << " took " << milliseconds << "." << remaining
              << "ms";
  Reset();
}