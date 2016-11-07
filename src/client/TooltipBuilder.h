#ifndef TOOLTIP_BUILDER_H
#define TOOLTIP_BUILDER_H

#include <SDL_ttf.h>
#include <vector>
#include <string>

#include "Texture.h"
#include "../Color.h"
#include "../types.h"

class TooltipBuilder{
    TTF_Font *_font;
    Color _color;
    std::vector<Texture> _content; // The lines of text; an empty texture implies a gap.

    static const px_t PADDING;
    static TTF_Font *_defaultFont;
    const static px_t DEFAULT_MAX_WIDTH;

public:
    TooltipBuilder();

    const static px_t NO_WRAP;

    void setFont(TTF_Font *font = nullptr); // Default: default font
    void setColor(const Color &color = Color::TOOLTIP_FONT);
    void addLine(const std::string &line);
    void addGap();
    Texture publish();

    // Create a basic tooltip containing a single string, broken over multiple lines if necessary.
    // maxWidth: the maximum width of the tooltip excluding padding; text will be wrapped to fit.
    // A maxWidth of NO_WRAP will result in no wrapping.
    static Texture basicTooltip(const std::string &text, px_t maxWidth = DEFAULT_MAX_WIDTH);
};

#endif