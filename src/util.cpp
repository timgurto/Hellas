#include "util.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "Args.h"
#include "Point.h"

using namespace std::string_literals;

extern Args cmdLineArgs;

bool isDebug() {
  const static bool debug = cmdLineArgs.contains("debug");
  return debug;
}

bool almostEquals(double a, double b) { return abs(a - b) < 0.001; }

double distance(const MapPoint &a, const MapPoint &b) {
  return sqrt(distanceSquared(a, b));
}

double distanceSquared(const MapPoint &a, const MapPoint &b) {
  auto xDelta = a.x - b.x, yDelta = a.y - b.y;
  return xDelta * xDelta + yDelta * yDelta;
}

double distance(const MapPoint &p, const MapPoint &a, const MapPoint &b) {
  if (a == b) return distance(p, a);
  double numerator =
      (b.y - a.y) * p.x - (b.x - a.x) * p.y + b.x * a.y - b.y * a.x;
  numerator = abs(numerator);
  double denominator = (b.y - a.y) * (b.y - a.y) + (b.x - a.x) * (b.x - a.x);

  if (denominator == 0) return 0;

  denominator = sqrt(denominator);
  return numerator / denominator;
}

MapPoint midpoint(const MapPoint &a, const MapPoint &b) {
  return {(a.x + b.x) / 2, (a.y + b.y) / 2};
}

MapPoint interpolate(const MapPoint &a, const MapPoint &b, double dist) {
  const double xDelta = b.x - a.x, yDelta = b.y - a.y;
  if (xDelta == 0 && yDelta == 0) return a;
  const double lengthAB = sqrt(xDelta * xDelta + yDelta * yDelta);

  if (lengthAB == 0) return a;

  if (dist >= lengthAB)
    // Target point exceeds b
    return b;

  const double xNorm = xDelta / lengthAB, yNorm = yDelta / lengthAB;
  return {a.x + xNorm * dist, a.y + yNorm * dist};
}

MapPoint extrapolate(const MapPoint &a, const MapPoint &b, double dist) {
  const double xDelta = b.x - a.x, yDelta = b.y - a.y;
  if (xDelta == 0 && yDelta == 0) return a;
  const double lengthAB = sqrt(xDelta * xDelta + yDelta * yDelta);

  if (lengthAB == 0) return a;

  if (dist <= lengthAB)
    // Target point is within interval
    return b;

  const double xNorm = xDelta / lengthAB, yNorm = yDelta / lengthAB;
  return {a.x + xNorm * dist, a.y + yNorm * dist};
}

MapPoint getRandomPointInCircle(const MapPoint &centre, double radius) {
  auto p = centre;
  if (radius != 0) {
    radius *= sqrt(randDouble());
    double angle = randDouble() * 2 * PI;
    p.x += cos(angle) * radius;
    p.y -= sin(angle) * radius;
  }
  return p;
}

std::string sAsTimeDisplay(int t) {
  auto remaining = t;
  auto seconds = remaining % 60;
  remaining /= 60;
  auto minutes = remaining % 60;
  remaining /= 60;
  auto hours = remaining % 24;
  remaining /= 24;
  auto days = remaining;

  auto oss = std::ostringstream{};
  if (days > 0) oss << days << "d "s;
  if (hours > 0) oss << hours << "h "s;
  if (minutes > 0) oss << minutes << "m "s;
  if (seconds > 0) oss << seconds << "s "s;
  auto timeDisplay = oss.str();
  timeDisplay =
      timeDisplay.substr(0, timeDisplay.size() - 1);  // Remove trailing space
  if (!timeDisplay.empty()) return timeDisplay;
  return "0s"s;
}

std::string msAsTimeDisplay(ms_t t) { return sAsTimeDisplay((t + 999) / 1000); }

std::string sAsShortTimeDisplay(int t) {
  if (t <= 60) {
    return toString(t) + "s";
  }

  if (t <= 3600) {
    auto minutes = t / 60;
    return toString(minutes) + "m";
  }

  auto hours = t / 3600;
  return toString(hours) + "h";
}

std::string msAsShortTimeDisplay(ms_t t) {
  return sAsShortTimeDisplay(t / 1000);
}

bool fileExists(const std::string &path) {
  std::ifstream fs(path);
  return fs.good();
}

bool isUsernameValid(const std::string name) {
  const auto MIN_CHARS = 3, MAX_CHARS = 20;
  if (name.length() < MIN_CHARS) return false;
  if (name.length() > MAX_CHARS) return false;
  for (char c : name)
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) return false;
  return true;
}

std::string timestamp() {
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%H:%M:%S") << " ";
  return oss.str();
}

std::string toPascal(std::string s) {
  if (s.empty()) return s;

  s[0] = toupper(s[0]);
  for (auto i = size_t{1}; i < s.size(); ++i) s[i] = tolower(s[i]);

  return s;
}

std::string toLower(std::string in) {
  for (auto &c : in) c = tolower(c);
  return in;
}

double getTameChanceBasedOnHealthPercent(double healthPercent) {
  if (healthPercent > 0.5) return 0;
  return (0.5 - healthPercent) * 2;
}

std::string proportionToPercentageString(double d) {
  d *= 100;
  return toString(toInt(d)) + "%";
}

std::string multiplicativeToString(double d) {
  d -= 1;
  return proportionToPercentageString(d);
}

int roll() { return rand() % 100 + 1; }

std::set<std::string> getXMLFiles(std::string path, std::string toExclude) {
  auto list = std::set<std::string>{};

  WIN32_FIND_DATA fd;
  path += "/"s;
  std::replace(path.begin(), path.end(), '/', '\\');
  std::string filter = path + "*.xml";
  path.c_str();
  HANDLE hFind = FindFirstFile(filter.c_str(), &fd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (fd.cFileName == toExclude) continue;
      auto file = path + fd.cFileName;
      list.insert(file);
    } while (FindNextFile(hFind, &fd));
    FindClose(hFind);
  }

  return list;
}

Color getDifficultyColor(Level contentLevel, Level playerLevel) {
  const auto difficulty = contentLevel - playerLevel;
  if (difficulty <= -10) return Color::DIFFICULTY_VERY_LOW;
  if (difficulty <= -5) return Color::DIFFICULTY_LOW;
  if (difficulty <= 4) return Color::DIFFICULTY_NEUTRAL;
  if (difficulty <= 9) return Color::DIFFICULTY_HIGH;
  return Color::DIFFICULTY_VERY_HIGH;
}
