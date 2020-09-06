#ifndef UTIL_H
#define UTIL_H

#include <cstdlib>
#include <set>
#include <sstream>
#include <vector>

#include "Point.h"
#include "Rect.h"
#include "messageCodes.h"

const double PI = 3.1415926535897;
const double SQRT_2 = 1.4142135623731;

bool isDebug();

inline int toInt(double d) {
  if (d > 0)
    return static_cast<int>(d + .5);
  else
    return static_cast<int>(d - .5);
}

inline double randDouble() { return (1.0 * rand()) / RAND_MAX; }

bool almostEquals(double a, double b);

template <typename T>
void pushBackMultiple(std::vector<T> &vec, const T &val, size_t count) {
  for (size_t i = 0; i != count; ++i) vec.push_back(val);
}

double distance(const MapPoint &a, const MapPoint &b);
double distance(const MapPoint &p, const MapPoint &a,
                const MapPoint &b);  // point P to line AB

MapPoint midpoint(const MapPoint &a, const MapPoint &b);

// Return the point between a and b which is dist from point a,
// or return b if dist exceeds the distance between a and b.
MapPoint interpolate(const MapPoint &a, const MapPoint &b, double dist);

// dist must exceed the distance between a and b.
MapPoint extrapolate(const MapPoint &a, const MapPoint &b, double dist);

MapPoint getRandomPointInCircle(const MapPoint &centre, double radius);

inline int str2int(const std::string str) {
  std::istringstream iss(str);
  int i;
  iss >> i;
  return i;
}

template <typename T>
std::string toString(T val) {
  std::ostringstream oss;
  oss << val;
  return oss.str();
}

template <typename T1>
std::string makeArgs(T1 val1) {
  return toString(val1);
}

template <typename T1, typename T2>
std::string makeArgs(T1 val1, T2 val2) {
  std::ostringstream oss;
  oss << val1 << MSG_DELIM << val2;
  return oss.str();
}

template <typename T1, typename T2, typename T3>
std::string makeArgs(T1 val1, T2 val2, T3 val3) {
  std::ostringstream oss;
  oss << val1 << MSG_DELIM << val2 << MSG_DELIM << val3;
  return oss.str();
}

template <typename T1, typename T2, typename T3, typename T4>
std::string makeArgs(T1 val1, T2 val2, T3 val3, T4 val4) {
  std::ostringstream oss;
  oss << val1 << MSG_DELIM << val2 << MSG_DELIM << val3 << MSG_DELIM << val4;
  return oss.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
std::string makeArgs(T1 val1, T2 val2, T3 val3, T4 val4, T5 val5) {
  std::ostringstream oss;
  oss << val1 << MSG_DELIM << val2 << MSG_DELIM << val3 << MSG_DELIM << val4
      << MSG_DELIM << val5;
  return oss.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6>
std::string makeArgs(T1 val1, T2 val2, T3 val3, T4 val4, T5 val5, T6 val6) {
  std::ostringstream oss;
  oss << val1 << MSG_DELIM << val2 << MSG_DELIM << val3 << MSG_DELIM << val4
      << MSG_DELIM << val5 << MSG_DELIM << val6;
  return oss.str();
}

#undef min
#undef max
template <typename T>
T min(const T &lhs, const T &rhs) {
  return lhs < rhs ? lhs : rhs;
}

template <typename T>
T max(const T &lhs, const T &rhs) {
  return rhs < lhs ? lhs : rhs;
}

std::string sAsTimeDisplay(int t);
std::string msAsTimeDisplay(ms_t t);
std::string sAsShortTimeDisplay(int t);
std::string msAsShortTimeDisplay(ms_t t);

bool fileExists(const std::string &path);

bool isUsernameValid(const std::string name);

std::string timestamp();

std::string toPascal(std::string in);

std::string toLower(std::string in);

double getTameChanceBasedOnHealthPercent(double healthPercent);

std::string proportionToPercentageString(double d);
std::string multiplicativeToString(double d);

int roll();

std::set<std::string> getXMLFiles(std::string path, std::string toExclude);

#endif
