#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include <thread>

// This class provides a task queue.  Any thread can enqueue a task, and a
// single thread will execute each in turn.
class WorkerThread {
 public:
  using Task = std::function<void()>;

  WorkerThread(const std::string threadName);
  void callBlocking(Task task);

 private:
  void run();
  Task _pendingTask;
  std::mutex _mutex;
};
