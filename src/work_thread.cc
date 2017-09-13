#include "work_thread.h"

#include "platform.h"

std::atomic<int> WorkThread::num_active_threads;
std::atomic<bool> WorkThread::request_exit_on_idle;

// static
void WorkThread::StartThread(
    const std::string& thread_name,
    const std::function<Result()>& entry_point) {
  new std::thread([thread_name, entry_point]() {
    SetCurrentThreadName(thread_name);

    ++num_active_threads;

    // Main loop.
    while (true) {
      Result result = entry_point();
      if (result == Result::ExitThread)
        break;
      if (request_exit_on_idle && result == Result::NoWork)
        break;
    }

    --num_active_threads;
  });
}