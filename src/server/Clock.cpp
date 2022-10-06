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

int DayChangeClock::currentDay() {
  auto rawTime = time_t{};
  auto localTime = tm{};
  localtime_s(&localTime, &rawTime);
  return localTime.tm_yday;
}
