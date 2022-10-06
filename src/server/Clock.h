#pragma once

#include <ctime>

class XmlWriter;
class XmlReader;

class DayChangeClock {
 public:
  DayChangeClock();

  bool hasDayChanged();

 private:
  int currentDay();
  int m_dayWhenLastChecked;
};
