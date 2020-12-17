#include "WorkerThread.h"

#include <SDL.h>

#include <cassert>

#include "threadNaming.h"

WorkerThread::WorkerThread(const std::string threadName) {
  std::thread([this, threadName]() {
    setThreadName(threadName);
    run();
  }).detach();
}

void WorkerThread::callBlocking(Task task) {
  _mutex.lock();

  assert(!_pendingTask);
  _pendingTask = task;

  while (_pendingTask)
    ;

  _mutex.unlock();
}

void WorkerThread::run() {
  while (true) {
    if (!_pendingTask) {
      continue;
    }

    _pendingTask();
    _pendingTask = {};
  }
}
