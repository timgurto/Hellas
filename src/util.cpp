#include <cassert>
#include <cmath>
#include <sstream>

#include "Args.h"
#include "Point.h"
#include "util.h"

using namespace std::string_literals;

extern Args cmdLineArgs;

bool isDebug(){
    const static bool debug = cmdLineArgs.contains("debug");
    return debug;
}

double distance(const MapPoint &a, const MapPoint &b){
    double
        xDelta = a.x - b.x,
        yDelta = a.y - b.y;
    return sqrt(xDelta * xDelta + yDelta * yDelta);
}

double distance(const MapPoint &p, const MapPoint &a, const MapPoint &b){
    if (a == b)
        return distance(p, a);
    double numerator = (b.y - a.y) * p.x -
                       (b.x - a.x) * p.y +
                       b.x * a.y -
                       b.y * a.x;
    numerator = abs(numerator);
    double denominator = (b.y - a.y) * (b.y - a.y) +
                         (b.x - a.x) * (b.x - a.x);
    assert (denominator > 0);
    denominator = sqrt(denominator);
    return numerator / denominator;
}

MapPoint midpoint(const MapPoint &a, const MapPoint &b){
    return{
        (a.x + b.x) / 2,
        (a.y + b.y) / 2
    };
}

MapPoint interpolate(const MapPoint &a, const MapPoint &b, double dist){
    const double
        xDelta = b.x - a.x,
        yDelta = b.y - a.y;
    if (xDelta == 0 && yDelta == 0)
        return a;
    const double lengthAB = sqrt(xDelta * xDelta + yDelta * yDelta);
    assert (lengthAB > 0);

    if (dist >= lengthAB)
        // Target point exceeds b
        return b;

    const double
        xNorm = xDelta / lengthAB,
        yNorm = yDelta / lengthAB;
    return{ a.x + xNorm * dist, a.y + yNorm * dist };
}

MapPoint extrapolate(const MapPoint & a, const MapPoint & b, double dist) {
    const double
        xDelta = b.x - a.x,
        yDelta = b.y - a.y;
    if (xDelta == 0 && yDelta == 0)
        return a;
    const double lengthAB = sqrt(xDelta * xDelta + yDelta * yDelta);
    assert(lengthAB > 0);

    if (dist <= lengthAB)
        // Target point is within interval
        return b;

    const double
        xNorm = xDelta / lengthAB,
        yNorm = yDelta / lengthAB;
    return{ a.x + xNorm * dist, a.y + yNorm * dist };
}

std::string sAsTimeDisplay(int t) {
    auto remaining = t;
    auto seconds = remaining % 60; remaining /= 60;
    auto minutes = remaining % 60; remaining /= 60;
    auto hours = remaining % 24; remaining /= 24;
    auto days = remaining;

    auto oss = std::ostringstream{};
    if (days > 0) oss << days << "d "s;
    if (hours > 0) oss << hours << "h "s;
    if (minutes > 0) oss << minutes << "m "s;
    if (seconds > 0) oss << seconds << "s "s;
    auto timeDisplay = oss.str();
    timeDisplay = timeDisplay.substr(0, timeDisplay.size() - 1); // Remove trailing space
    if (!timeDisplay.empty())
        return timeDisplay;
    return "0s"s;
}

std::string msAsTimeDisplay(ms_t t) {
    return sAsTimeDisplay(t/1000);
}
