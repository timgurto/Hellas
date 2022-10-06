#include "Clock.h"

bool DayChangeClock::hasDayChanged() { return false; }

void DayChangeClock::backup(XmlWriter &xw) const {}

void DayChangeClock::restore(const XmlReader &xr) {}
