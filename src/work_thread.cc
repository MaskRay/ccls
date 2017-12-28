#include "work_thread.h"

#include "platform.h"

// static
void WorkThread::StartThread(const std::string& thread_name,
                             std::function<void()> entry_point) {
  new std::thread([thread_name, entry_point]() {
    SetCurrentThreadName(thread_name);
    entry_point();
  });
}