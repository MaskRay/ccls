#include "timer.h"

#include <loguru.hpp>

Timer::Timer() {
  Reset();
}

long long Timer::ElapsedMicroseconds() const {
  std::chrono::time_point<Clock> end = Clock::now();
  return elapsed_ +
         std::chrono::duration_cast<std::chrono::microseconds>(end - start_)
             .count();
}

long long Timer::ElapsedMicrosecondsAndReset() {
  long long elapsed = ElapsedMicroseconds();
  Reset();
  return elapsed;
}

void Timer::Reset() {
  start_ = Clock::now();
  elapsed_ = 0;
}

void Timer::ResetAndPrint(const std::string& message) {
  long long elapsed = ElapsedMicroseconds();
  long long milliseconds = elapsed / 1000;
  long long remaining = elapsed - milliseconds;
  LOG_S(INFO) << message << " took " << milliseconds << "." << remaining
              << "ms";
  Reset();
}

void Timer::Pause() {
  std::chrono::time_point<Clock> end = Clock::now();
  long long elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start_)
          .count();

  elapsed_ += elapsed;
}

void Timer::Resume() {
  start_ = Clock::now();
}

ScopedPerfTimer::~ScopedPerfTimer() {
  timer_.ResetAndPrint(message_);
}
