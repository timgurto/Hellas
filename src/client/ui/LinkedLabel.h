#ifndef LINKED_LABEL_H
#define LINKED_LABEL_H

#include <sstream>
#include <string>
#include "Label.h"

// Contains a Label, but its contents are linked to a variable.
template <typename T>
class LinkedLabel : public Label {
  const T &_val;
  T _lastCheckedVal;
  std::string _prefix, _suffix;

 public:
  LinkedLabel(const ScreenRect &rect, const T &val,
              const std::string &prefix = "", const std::string &suffix = "",
              Justification justificationH = LEFT_JUSTIFIED,
              Justification justificationV = TOP_JUSTIFIED);

 private:
  virtual void checkIfChanged() override;
  void updateText();
};

template <typename T>
LinkedLabel<T>::LinkedLabel(const ScreenRect &rect, const T &val,
                            const std::string &prefix,
                            const std::string &suffix,
                            Justification justificationH,
                            Justification justificationV)
    : Label(rect, "", justificationH, justificationV),
      _val(val),
      _lastCheckedVal(val),
      _prefix(prefix),
      _suffix(suffix) {
  updateText();
}

template <typename T>
void LinkedLabel<T>::updateText() {
  std::ostringstream oss;
  if (!_prefix.empty()) oss << _prefix;
  oss << _val;
  if (!_suffix.empty()) oss << _suffix;
  _text = oss.str();
}

template <typename T>
void LinkedLabel<T>::checkIfChanged() {
  if (_val != _lastCheckedVal) {
    _lastCheckedVal = _val;

    updateText();

    markChanged();
    Label::checkIfChanged();
  }
}

#endif
