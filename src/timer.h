#pragma once

#include <chrono>

struct Timer {
  using Clock = std::chrono::high_resolution_clock;

  // Creates a new timer. A timer is always running.
  Timer();

  // Restart/reset the timer.
  void Reset();

  // Return the number of milliseconds since the timer was last reset.
  long long ElapsedMilliseconds();

  // Raw start time.
  std::chrono::time_point<Clock> start;
};
