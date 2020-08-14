#include "WorkerThread.h"

#include <SDL.h>

#include <cassert>

#include "../threadNaming.h"

WorkerThread::WorkerThread(const std::string threadName) {
  std::thread([this, threadName]() {
    setThreadName(threadName);
    run();
  }).detach();
}

void WorkerThread::callBlocking(Task task) {
  auto scheduledTask = ScheduledTask{};
  scheduledTask.serial = generateSerial();
  scheduledTask.task = task;

  _queueMutex.lock();
  _pendingTasks.push(scheduledTask);
  _queueMutex.unlock();

  while (!isTaskFinished(scheduledTask.serial))
    ;
}

void WorkerThread::run() {
  while (true) {
    if (_pendingTasks.empty()) {
      SDL_Delay(1);
      continue;
    }

    _pendingTasks.front().task();
    _queueMutex.lock();
    _pendingTasks.pop();
    _queueMutex.unlock();
  }
}

int WorkerThread::generateSerial() { return ++_nextSerial; }

bool WorkerThread::isTaskFinished(int serial) const {
  _queueMutex.lock();

  if (_pendingTasks.empty()) {
    _queueMutex.unlock();
    return true;
  }

  auto taskCurrentlyRunning = _pendingTasks.front().serial;
  _queueMutex.unlock();

  return serial < taskCurrentlyRunning;
}
