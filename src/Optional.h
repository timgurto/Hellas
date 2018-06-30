#pragma once

#include <cassert>
#include <memory>

template <typename T>
class Optional {
 public:
  Optional() : _p{nullptr} {}

  Optional(const Optional &rhs) {
    if (rhs.hasValue()) _p = new T(rhs.value());
  }

  ~Optional() {
    if (_p) delete _p;
  }

  const T &operator=(const T &rhs) {
    if (hasValue()) delete _p;
    _p = new T(rhs);

    return *_p;
  }

  bool hasValue() const { return _p != nullptr; }

  const T &value() const {
    assert(_p);
    return *_p;
  }

  T &value() {
    assert(_p);
    return *_p;
  }

 private:
  T *_p{nullptr};
};
