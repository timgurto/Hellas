#include "Point.h"
#include "util.h"

MapPoint toMapPoint(const ScreenPoint &rhs) {
    return{
        static_cast<double>(rhs.x),
        static_cast<double>(rhs.y)
    };
}

ScreenPoint toScreenPoint(const MapPoint &rhs) {
    return{
        toInt(rhs.x),
        toInt(rhs.y)
    };
}
