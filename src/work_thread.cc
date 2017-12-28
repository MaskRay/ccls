#include "work_thread.h"

#include "platform.h"

// static
void WorkThread::StartThread(const std::string& thread_name,
                             const std::function<Result()>& entry_point) {
  new std::thread([thread_name, entry_point]() {
    SetCurrentThreadName(thread_name);

    // Main loop.
    while (true) {
      Result result = entry_point();
      if (result == Result::ExitThread)
        break;
    }
  });
}