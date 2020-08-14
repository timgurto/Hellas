#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include <thread>

// SDL requires all calls to be done in one thread.  This class enables that, by
// providing a task queue.  Any thread can enqueue a task, and a single thread
// will execute each in turn.
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
