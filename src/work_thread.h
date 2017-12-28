#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

// Helper methods for starting threads that do some work. Enables test code to
// wait for all work to complete.
struct WorkThread {
  // FIXME: remove result, have entry_point run a while(true) loop.
  enum class Result { MoreWork, NoWork, ExitThread };

  // Launch a new thread. |entry_point| will be called continously. It should
  // return true if it there is still known work to be done.
  static void StartThread(const std::string& thread_name,
                          const std::function<Result()>& entry_point);

  // Static-only class.
  WorkThread() = delete;
};