#pragma once

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

    if (!_p) {
      // Default value in lieu of null, to avoid the need for asserts.
      static T dummy{};
      return dummy;
    }

    return *_p;
  }

  bool hasValue() const { return _p != nullptr; }

  const T &value() const { return *_p; }
  T &value() {
    if (!_p) {
      // Default value in lieu of null, to avoid the need for asserts.
      static T dummy{};
      return dummy;
    }

    return *_p;
  }

 private:
  T *_p{nullptr};
};
