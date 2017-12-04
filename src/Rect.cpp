#include "Rect.h"
#include "util.h"

ScreenRect toScreenRect(const MapRect &rhs) {
    return{
        toInt(rhs.x),
        toInt(rhs.y),
        toInt(rhs.w),
        toInt(rhs.h)
    };
}
