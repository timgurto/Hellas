#pragma once

#include <ctime>

class DayChangeClock {
 public:
  DayChangeClock();

  bool hasDayChanged();

  static bool hasDayChangedSince(time_t previousTime);

 private:
  static int currentDay();
  int m_dayWhenLastChecked;
};
