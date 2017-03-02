#ifndef COLOR_H
#define COLOR_H

#include "SDL.h"

class Color{
public:
    static const Color BLACK;
    static const Color BLUE;
    static const Color GREEN;
    static const Color CYAN;
    static const Color RED;
    static const Color MAGENTA;
    static const Color YELLOW;
    static const Color WHITE;

    static Color
        WARNING,
        FAILURE,
        SUCCESS,

        CHAT_LOG_BACKGROUND,
        SAY,
        WHISPER,

        DEFAULT_DRAW,
        FONT,
        FONT_OUTLINE,

        TOOLTIP_FONT,
        TOOLTIP_BACKGROUND,
        TOOLTIP_BORDER,

        ITEM_NAME,
        ITEM_STATS,
        ITEM_INSTRUCTIONS,
        ITEM_TAGS,

        ELEMENT_BACKGROUND,
        ELEMENT_SHADOW_DARK,
        ELEMENT_SHADOW_LIGHT,
        ELEMENT_FONT,
        CONTAINER_SLOT_BACKGROUND,

        FOOTPRINT_GOOD,
        FOOTPRINT_BAD,
        FOOTPRINT,
        IN_RANGE,
        OUT_OF_RANGE,
        HEALTH_BAR,
        HEALTH_BAR_BACKGROUND,
        HEALTH_BAR_OUTLINE,
        CAST_BAR_FONT,
        PERFORMANCE_FONT,
        PROGRESS_BAR,
        PROGRESS_BAR_BACKGROUND,
        PLAYER_NAME,
        PLAYER_NAME_OUTLINE,

        OUTLINE,
        HIGHLIGHT_OUTLINE;

    static const Color GREY_2;
    static const Color GREY_4;
    static const Color GREY_8;

    //static const Color BLUE_HELL;
    static const Color NO_KEY;

    Color(Uint8 r = 0, Uint8 g = 0, Uint8 b = 0);
    Color(const SDL_Color &rhs);
    Color(Uint32 rhs);

    operator SDL_Color() const;
    operator Uint32() const; // Used by some SDL functions

    // Multiply or divide color by a scalar
    Color operator/(double s) const;
    Color operator/(int s) const; // to avoid ambiguity with implicit Uint32 cast
    Color operator*(double s) const;
    Color operator*(int s) const; // to avoid ambiguity with implicit Uint32 cast

    Uint8 r() const { return _r; }
    Uint8 g() const { return _g; }
    Uint8 b() const { return _b; }

private:
    Uint8 _r, _g, _b;
};

Color operator+(const Color &lhs, const Color &rhs);

#endif