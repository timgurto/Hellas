#pragma once

class XmlWriter;
class XmlReader;

class IDayChangeClock {
 public:
  virtual bool hasDayChanged() = 0;
  virtual void backup(XmlWriter &xw) const {};
  virtual void restore(const XmlReader &xr){};
};

class DayChangeClock : public IDayChangeClock {
  bool hasDayChanged() override;
  void backup(XmlWriter &xw) const override;
  void restore(const XmlReader &xr) override;
};

#ifdef TESTING
class MockDayChangeClock : public IDayChangeClock {
 public:
  bool hasDayChanged() override {
    const auto ret = m_hasDayChanged;
    m_hasDayChanged = false;
    return ret;
  }

  void changeDay() { m_hasDayChanged = true; }

 private:
  bool m_hasDayChanged{false};
};
#endif
