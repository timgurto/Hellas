#include "Clock.h"

DayChangeClock::DayChangeClock() : m_dayWhenLastChecked(currentDay()) {}

bool DayChangeClock::hasDayChanged() {
  const auto thisDay = currentDay();
  if (thisDay != m_dayWhenLastChecked) {
    m_dayWhenLastChecked = thisDay;
    return true;
  }

  return false;
}

bool DayChangeClock::hasDayChangedSince(time_t previousTime) {
  auto localTime = tm{};
  localtime_s(&localTime, &previousTime);
  auto previousDay = localTime.tm_yday;

  return previousDay != currentDay();
}

int DayChangeClock::currentDay() {
  auto rawTime = time(0);
  auto localTime = tm{};
  localtime_s(&localTime, &rawTime);
  return localTime.tm_yday;
}
