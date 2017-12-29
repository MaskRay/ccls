#if false
#pragma once

#include <optional.h>

#include <chrono>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

enum class TaskThread {
  Indexer,
  QueryDb,
};

struct TaskManager {
  using TTask = std::function<void()>;
  using TIdleTask = std::function<bool()>;

  TaskManager();

  // Run |task| at some point in the future. This will run the task as soon as possible.
  void Post(TaskThread thread, const TTask& task);

  // Run |task| whenever there is nothing else to run.
  void SetIdle(TaskThread thread, const TIdleTask& idle_task);

  // Run pending tasks for |thread|. Stop running tasks after |max_time| has
  // elapsed. Returns true if tasks were run.
  bool RunTasks(TaskThread thread, optional<std::chrono::duration<long long, std::nano>> max_time);

  struct TaskQueue {
    optional<TIdleTask> idle_task;
    std::vector<TTask> tasks;
    std::mutex tasks_mutex;
  };

  std::unordered_map<TaskThread, std::unique_ptr<TaskQueue>> pending_tasks_;
};
#endif