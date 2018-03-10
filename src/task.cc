#if false
#include "task.h"

#include "utils.h"

#include <doctest/doctest.h>

TaskManager::TaskManager() {
  pending_tasks_[TaskThread::Indexer] = std::make_unique<TaskQueue>();
  pending_tasks_[TaskThread::QueryDb] = std::make_unique<TaskQueue>();
}

void TaskManager::Post(TaskThread thread, const TTask& task) {
  TaskQueue* queue = pending_tasks_[thread].get();
  std::lock_guard<std::mutex> lock_guard(queue->tasks_mutex);
  queue->tasks.push_back(task);
}

void TaskManager::SetIdle(TaskThread thread, const TIdleTask& task) {
  TaskQueue* queue = pending_tasks_[thread].get();
  std::lock_guard<std::mutex> lock_guard(queue->tasks_mutex);
  assert(!queue->idle_task && "There is already an idle task");
  queue->idle_task = task;
}

bool TaskManager::RunTasks(TaskThread thread, optional<std::chrono::duration<long long, std::nano>> max_time) {
  auto start = std::chrono::high_resolution_clock::now();
  TaskQueue* queue = pending_tasks_[thread].get();

  bool ran_task = false;

  while (true) {
    optional<TTask> task;

    // Get a task.
    {
      std::lock_guard<std::mutex> lock_guard(queue->tasks_mutex);
      if (queue->tasks.empty())
        break;
      task = std::move(queue->tasks[queue->tasks.size() - 1]);
      queue->tasks.pop_back();
    }
    
    // Execute task.
    assert(task);
    (*task)();
    ran_task = true;

    // Stop if we've run past our max time. Don't run idle_task.
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    if (max_time && elapsed > *max_time)
      return ran_task;
  }

  if (queue->idle_task) {
    // Even if the idle task returns false we still ran something before.
    ran_task = (*queue->idle_task)() || ran_task;
  }

  return ran_task;
}

TEST_SUITE("Task");

TEST_CASE("tasks are run as soon as they are posted") {
  TaskManager tm;
  
  // Post three tasks.
  int next = 1;
  int a = 0, b = 0, c = 0;
  tm.Post(TaskThread::QueryDb, [&] {
    a = next++;
  });
  tm.Post(TaskThread::QueryDb, [&] {
    b = next++;
  });
  tm.Post(TaskThread::QueryDb, [&] {
    c = next++;
  });

  // Execute all tasks.
  tm.RunTasks(TaskThread::QueryDb, nullopt);

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
  tm.Post(TaskThread::QueryDb, [&] () {
    a = next++;

    tm.Post(TaskThread::QueryDb, [&] {
      b = next++;

      tm.Post(TaskThread::QueryDb, [&] {
        c = next++;
      });
    });
  });

  // Execute all tasks.
  tm.RunTasks(TaskThread::QueryDb, nullopt);

  // Tasks are executed in normal order because the next task is not posted
  // until the previous one is executed.
  REQUIRE(a == 1);
  REQUIRE(b == 2);
  REQUIRE(c == 3);
}

TEST_CASE("idle task is run after nested tasks") {
  TaskManager tm;

  int count = 0;
  tm.SetIdle(TaskThread::QueryDb, [&]() {
    ++count;
    return true;
  });

  // No tasks posted - idle runs once.
  REQUIRE(tm.RunTasks(TaskThread::QueryDb, nullopt));
  REQUIRE(count == 1);
  count = 0;

  // Idle runs after other posted tasks.
  bool did_run = false;
  tm.Post(TaskThread::QueryDb, [&]() {
    did_run = true;
  });
  REQUIRE(tm.RunTasks(TaskThread::QueryDb, nullopt));
  REQUIRE(did_run);
  REQUIRE(count == 1);
}

TEST_CASE("RunTasks returns false when idle task returns false and no other tasks were run") {
  TaskManager tm;

  REQUIRE(tm.RunTasks(TaskThread::QueryDb, nullopt) == false);
  
  tm.SetIdle(TaskThread::QueryDb, []() { return false; });
  REQUIRE(tm.RunTasks(TaskThread::QueryDb, nullopt) == false);
}

TEST_SUITE_END();
#endif