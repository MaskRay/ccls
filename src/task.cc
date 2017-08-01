#include "utils.h"

#include <doctest/doctest.h>
#include <optional.h>

#include <chrono>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

using std::experimental::optional;
using std::experimental::nullopt;

enum class TaskTargetThread {
  Indexer,
  QueryDb,
};

// TODO: IdleTask returns a bool indicating if it did work.
// TODO: Hookup IdleTask
// TODO: Move target_thread out of task and into PostTask.

struct Task {
  // The thread the task will execute on.
  TaskTargetThread target_thread;

  // The action the task will perform.
  using TAction = std::function<void()>;
  TAction action;

  Task(TaskTargetThread target, const TAction& action);
};

Task::Task(TaskTargetThread target, const TAction& action)
  : target_thread(target), action(action) {}


struct TaskQueue {
  optional<Task> idle_task;
  std::vector<Task> tasks;
  std::mutex tasks_mutex;
};

struct TaskManager {
  TaskManager();

  // Run |task| at some point in the future. This will run the task as soon as possible.
  void PostTask(const Task& task);

  // Run |task| whenever there is nothing else to run.
  void SetIdleTask(const Task& task);

  // Run pending tasks for |thread|. Stop running tasks after |max_time| has elapsed.
  void RunTasks(TaskTargetThread thread, optional<std::chrono::duration<long long, std::nano>> max_time);

  std::unordered_map<TaskTargetThread, std::unique_ptr<TaskQueue>> pending_tasks_;
};

TaskManager::TaskManager() {
  pending_tasks_[TaskTargetThread::Indexer] = MakeUnique<TaskQueue>();
  pending_tasks_[TaskTargetThread::QueryDb] = MakeUnique<TaskQueue>();
}

void TaskManager::PostTask(const Task& task) {
  TaskQueue* queue = pending_tasks_[task.target_thread].get();
  std::lock_guard<std::mutex> lock_guard(queue->tasks_mutex);
  queue->tasks.push_back(task);
}

void TaskManager::SetIdleTask(const Task& task) {
  TaskQueue* queue = pending_tasks_[task.target_thread].get();
  std::lock_guard<std::mutex> lock_guard(queue->tasks_mutex);
  assert(!queue->idle_task && "There is already an idle task");
  queue->idle_task = task;
}

void TaskManager::RunTasks(TaskTargetThread thread, optional<std::chrono::duration<long long, std::nano>> max_time) {
  auto start = std::chrono::high_resolution_clock::now();
  TaskQueue* queue = pending_tasks_[thread].get();

  while (true) {
    optional<Task> task;

    // Get a task.
    {
      std::lock_guard<std::mutex> lock_guard(queue->tasks_mutex);
      if (queue->tasks.empty())
        return;
      task = queue->tasks[queue->tasks.size() - 1];
      queue->tasks.pop_back();
    }
    
    // Execute task.
    assert(task);
    task->action();

    // If we've run past our max time stop.
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    if (max_time && elapsed > *max_time)
      return;
  }
}

TEST_SUITE("Task");

TEST_CASE("tasks are run as soon as they are posted") {
  TaskManager tm;
  
  // Post three tasks.
  int next = 1;
  int a = 0, b = 0, c = 0;
  tm.PostTask(Task(TaskTargetThread::QueryDb, [&] {
    a = next++;
  }));
  tm.PostTask(Task(TaskTargetThread::QueryDb, [&] {
    b = next++;
  }));
  tm.PostTask(Task(TaskTargetThread::QueryDb, [&] {
    c = next++;
  }));

  // Execute all tasks.
  tm.RunTasks(TaskTargetThread::QueryDb, nullopt);

  // Tasks are executed in reverse order.
  REQUIRE(a == 3);
  REQUIRE(b == 2);
  REQUIRE(c == 1);
}

TEST_CASE("post from inside task manager") {
  TaskManager tm;

  // Post three tasks.
  int next = 1;
  int a = 0, b = 0, c = 0;
  tm.PostTask(Task(TaskTargetThread::QueryDb, [&] () {
    a = next++;

    tm.PostTask(Task(TaskTargetThread::QueryDb, [&] {
      b = next++;

      tm.PostTask(Task(TaskTargetThread::QueryDb, [&] {
        c = next++;
      }));
    }));
  }));

  // Execute all tasks.
  tm.RunTasks(TaskTargetThread::QueryDb, nullopt);

  // Tasks are executed in normal order because the next task is not posted
  // until the previous one is executed.
  REQUIRE(a == 1);
  REQUIRE(b == 2);
  REQUIRE(c == 3);
}

TEST_SUITE_END();