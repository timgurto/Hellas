// (C) 2016 Tim Gurto

#include "NPC.h"

const Rect NPC::COLLISION_RECT(-9, -3, 18, 5);

NPC::NPC(const Point &loc):
_serial(generateSerial()),
_location(loc){
}

NPC::NPC(size_t serial): // For set/map lookup ONLY
_serial(serial){}

size_t NPC::generateSerial() {
    static size_t currentSerial = 1; // Serial 0 is unavailable, and has special meaning.
    return currentSerial++;
}
