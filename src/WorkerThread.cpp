#include "WorkerThread.h"

#include <SDL.h>

#include <cassert>

#include "threadNaming.h"

WorkerThread::WorkerThread(const std::string threadName) {
  _thread = std::thread([this, threadName]() {
    setThreadName(threadName);
    run();
  });
}

WorkerThread::~WorkerThread() {
  _running = false;
  _thread.join();
}

void WorkerThread::callBlocking(Task task) {
  _mutex.lock();

  assert(!_pendingTask);
  _pendingTask = task;

  while (_pendingTask);

  _mutex.unlock();
}

void WorkerThread::run() {
  while (_running) {
    if (!_pendingTask) {
      continue;
    }

    _pendingTask();
    _pendingTask = {};
  }
}
