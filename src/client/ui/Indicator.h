#pragma once

#include <unordered_map>

#include "Element.h"
#include "Picture.h"

// Illustrates the state of something--success, fail, or in progress
class Indicator : public Picture {
public:
    enum Status {
        FAILED,
        SUCCEEDED,
        IN_PROGRESS
    };

    Indicator(const ScreenPoint &loc, Status initial = IN_PROGRESS);
    void set(Status status);

    static void initialize();

private:
    using Images = std::unordered_map<Status, Texture>;
    static Images images;

    static bool initialized;
};
