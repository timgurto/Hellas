#ifndef TOOLTIP_BUILDER_H
#define TOOLTIP_BUILDER_H

#include <SDL_ttf.h>
#include <vector>
#include <string>

#include "Color.h"
#include "Texture.h"

class TooltipBuilder{
    TTF_Font *_font;
    Color _color;
    std::vector<Texture> _content; // The lines of text; an empty texture implies a gap.

    static const int PADDING;
    static TTF_Font *_defaultFont;
    static const Color DEFAULT_COLOR;
    static const Color BACKGROUND_COLOR;

public:
    TooltipBuilder();

    void setFont(TTF_Font *font = 0); // Default: default font
    void setColor(const Color &color = DEFAULT_COLOR);
    void addLine(const std::string &line);
    void addGap();
    void publish(Texture &target);
};

#endif