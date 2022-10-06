#pragma once

class XmlWriter;
class XmlReader;

class DayChangeClock {
 public:
  bool hasDayChanged();
  void backup(XmlWriter &xw) const;
  void restore(const XmlReader &xr);
};
